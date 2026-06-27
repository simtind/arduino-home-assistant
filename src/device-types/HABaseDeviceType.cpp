#include "HABaseDeviceType.h"
#include "../HAMqtt.h"
#include "../HADevice.h"
#include "../utils/HAUtils.h"
#include "../utils/HASerializer.h"

HABaseDeviceType::HABaseDeviceType(
    const __FlashStringHelper* componentName,
    const char* uniqueId
) :
    _componentName(componentName),
    _uniqueId(uniqueId)
{
}

HABaseDeviceType::HABaseDeviceType(
    HADevice & device,
    const __FlashStringHelper* componentName,
    const char* uniqueId
) : 
    _componentName(componentName),
    _uniqueId(uniqueId),
    _device(&device)
{
    _device->addDeviceType(this);
}

void HABaseDeviceType::setAvailability(bool online)
{
    _availability = (online ? AvailabilityOnline : AvailabilityOffline);
    publishAvailability();
}

void HABaseDeviceType::subscribeTopic(
    const char* uniqueId,
    const __FlashStringHelper* topic
)
{
    const uint16_t topicLength = HASerializer::calculateDataTopicLength(
        uniqueId,
        topic
    );
    if (topicLength == 0) {
        return;
    }

    char fullTopic[topicLength];
    if (!HASerializer::generateDataTopic(
        fullTopic,
        uniqueId,
        topic
    )) {
        return;
    }

    _device->mqtt()->subscribe(fullTopic);
}

void HABaseDeviceType::onMqttMessage(
    const char* topic,
    const uint8_t* payload,
    const uint16_t length
)
{
    (void)topic;
    (void)payload;
    (void)length;
}

void HABaseDeviceType::destroySerializer()
{
    if (_serializer) {
        delete _serializer;
        _serializer = nullptr;
    }
}

void HABaseDeviceType::publishConfig()
{
    buildSerializer();

    if (_serializer == nullptr) {
        return;
    }

    const uint16_t topicLength = HASerializer::calculateConfigTopicLength(
        componentName(),
        uniqueId()
    );
    const uint16_t dataLength = _serializer->calculateSize();

    if (topicLength > 0 && dataLength > 0) {
        char topic[topicLength];
        HASerializer::generateConfigTopic(
            topic,
            componentName(),
            uniqueId()
        );

        if (_device->mqtt()->beginPublish(topic, dataLength, true)) {
            _serializer->flush();
            _device->mqtt()->endPublish();
        }
    }

    destroySerializer();
}

void HABaseDeviceType::publishAvailability()
{
    if (
        !_device ||
        _device->isSharedAvailabilityEnabled() ||
        !isAvailabilityConfigured()
    ) {
        return;
    }

    publishOnDataTopic(
        AHATOFSTR(HAAvailabilityTopic),
        _availability == AvailabilityOnline
            ? AHATOFSTR(HAOnline)
            : AHATOFSTR(HAOffline),
        true
    );
}

bool HABaseDeviceType::publishOnDataTopic(
    const __FlashStringHelper* topic,
    const __FlashStringHelper* payload,
    bool retained
)
{
    if (!payload) {
        return false;
    }

    return publishOnDataTopic(
        topic,
        reinterpret_cast<const uint8_t*>(payload),
        strlen_P(AHAFROMFSTR(payload)),
        retained,
        true
    );
}

bool HABaseDeviceType::publishOnDataTopic(
    const __FlashStringHelper* topic,
    const char* payload,
    bool retained
)
{
    if (!payload) {
        return false;
    }

    return publishOnDataTopic(
        topic,
        reinterpret_cast<const uint8_t*>(payload),
        strlen(payload),
        retained
    );
}

bool HABaseDeviceType::publishOnDataTopic(
    const __FlashStringHelper* topic,
    const uint8_t* payload,
    const uint16_t length,
    bool retained,
    bool isProgmemData
)
{
    if (!payload) {
        return false;
    }

    const uint16_t topicLength = HASerializer::calculateDataTopicLength(
        uniqueId(),
        topic
    );
    if (topicLength == 0) {
        return false;
    }

    char fullTopic[topicLength];
    if (!HASerializer::generateDataTopic(
        fullTopic,
        uniqueId(),
        topic
    )) {
        return false;
    }

    if (_device->mqtt()->beginPublish(fullTopic, length, retained)) {
        if (isProgmemData) {
            _device->mqtt()->writePayload(AHATOFSTR(payload));
        } else {
            _device->mqtt()->writePayload(payload, length);
        }

        return _device->mqtt()->endPublish();
    }

    return false;
}