#include "ArduinoHADefines.h"
#include "HADevice.h"
#include "HAMqtt.h"
#include "utils/HAUtils.h"
#include "utils/HASerializer.h"


HADevice::HADevice(const uint8_t maxDevicesTypesNb) :
    _uniqueId(nullptr),
    _serializer(new HASerializer(nullptr, 6))
{
    _devicesTypes.reserve(maxDevicesTypesNb);
}

HADevice::HADevice(const char* uniqueId, const uint8_t maxDevicesTypesNb) :
    _uniqueId(uniqueId),
    _serializer(new HASerializer(nullptr, 6))
{
    _devicesTypes.reserve(maxDevicesTypesNb);
    _serializer->set(AHATOFSTR(HADeviceIdentifiersProperty), _uniqueId);
}

HADevice::HADevice(const byte* uniqueId, const uint16_t length, const uint8_t maxDevicesTypesNb) :
    _uniqueId(HAUtils::byteArrayToStr(uniqueId, length)),
    _serializer(new HASerializer(nullptr, 6)),

{
    _devicesTypes.reserve(maxDevicesTypesNb);
    _ownsUniqueId = true;
    _serializer->set(AHATOFSTR(HADeviceIdentifiersProperty), _uniqueId);
}

HADevice::~HADevice()
{
    delete _serializer;

    if (_availabilityTopic) {
        delete _availabilityTopic;
    }

    if (_ownsUniqueId) {
        delete[] _uniqueId;
    }
}

void HADevice::setMQtt(HAMqtt * mqtt)
{
    _mqtt = mqtt;
}

bool HADevice::setUniqueId(const byte* uniqueId, const uint16_t length)
{
    if (_uniqueId) {
        return false; // unique ID cannot be changed at runtime once it's set
    }

    _uniqueId = HAUtils::byteArrayToStr(uniqueId, length);
    _ownsUniqueId = true;
    _serializer->set(AHATOFSTR(HADeviceIdentifiersProperty), _uniqueId);
    return true;
}

void HADevice::setManufacturer(const char* manufacturer)
{
    _serializer->set(AHATOFSTR(HADeviceManufacturerProperty), manufacturer);
}

void HADevice::setModel(const char* model)
{
    _serializer->set(AHATOFSTR(HADeviceModelProperty), model);
}

void HADevice::setName(const char* name)
{
    _serializer->set(AHATOFSTR(HANameProperty), name);
}

void HADevice::setSoftwareVersion(const char* softwareVersion)
{
    _serializer->set(
        AHATOFSTR(HADeviceSoftwareVersionProperty),
        softwareVersion
    );
}

void HADevice::setConfigurationUrl(const char* url)
{
    _serializer->set(
        AHATOFSTR(HADeviceConfigurationUrlProperty),
        url
    );
}

void HADevice::setAvailability(bool online)
{
    _available = online;
    publishAvailability();
}

bool HADevice::enableSharedAvailability()
{
    if (_sharedAvailability) {
        return true; // already enabled
    }

    const uint16_t topicLength = HASerializer::calculateDataTopicLength(
        nullptr,
        AHATOFSTR(HAAvailabilityTopic)
    );
    if (topicLength == 0) {
        return false;
    }

    _availabilityTopic = new char[topicLength];

    if (HASerializer::generateDataTopic(
        _availabilityTopic,
        nullptr,
        AHATOFSTR(HAAvailabilityTopic)
    ) > 0) {
        _sharedAvailability = true;
        return true;
    }

    return false;
}

void HADevice::enableLastWill()
{
    if (!_mqtt || !_availabilityTopic) {
        return;
    }

    _mqtt->setLastWill(
        _availabilityTopic,
        "offline",
        true
    );
}

void HADevice::publishAvailability() const
{
    if (!_availabilityTopic || !_mqtt) {
        return;
    }

    const char* payload = _available ? HAOnline : HAOffline;
    const uint16_t length = strlen_P(payload);

    if (_mqtt->beginPublish(_availabilityTopic, length, true)) {
        _mqtt->writePayload(AHATOFSTR(payload));
        _mqtt->endPublish();
    }
}


void HADevice::addDeviceType(HABaseDeviceType* deviceType)
{
    if (_devicesTypes.size() == _devicesTypes.capacity()) {
        return;
    }

    _devicesTypes.push_back(deviceType);
}

void HADevice::setMaxDevicesTypesNb(uint8_t maxDevicesTypesNb)
{
    _deviceTypes.reserve(maxDevicesTypesNb);
}

void HADevice::onMqttConnected()
{
    for (HABaseDeviceType * type : _deviceTypes)
    {
        type->onMqttConnected();
    }
}

void HADevice::onMqttMessage(
    const char* topic,
    const uint8_t* payload,
    const uint16_t length
)
{
    for (HABaseDeviceType * type : _deviceTypes)
    {
        type->onMqttMessage(topic, payload, length);
    }
}