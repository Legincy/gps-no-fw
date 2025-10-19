#include "states/UpdateState.h"

void UpdateState::enter()
{
    log.info("UpdateState", "Entering Update State");
    currentPhase = UpdatePhase::CHECK_VERSION;
}

void UpdateState::update()
{
    if (!checkUpdateConditions())
    {
        device->changeState(ActionState::getInstance(device));
        return;
    }

    switch (currentPhase)
    {
    case UpdatePhase::CHECK_VERSION:
        if (checkLatestRelease())
        {
            currentPhase = UpdatePhase::DOWNLOAD;
        }
        else
        {
            // TODO: Jump back to RECENT/CALLER state
            device->changeState(ActionState::getInstance(device));
        }

        break;
    case UpdatePhase::DOWNLOAD:
        if (downloadAndInstall())
        {
            currentPhase = UpdatePhase::COMPLETED;
        }

        break;
    case UpdatePhase::INSTALL:
        break;
    case UpdatePhase::COMPLETED:
        if (Update.isFinished())
        {
            reportProgress("Update completed, restarting device...", 100);
            delay(1000);
            ESP.restart();
        }

        break;
    case UpdatePhase::FAILED:
        handleUpdateError("Update failed");
        device->changeState(ErrorState::getInstance(device));

        break;
    default:
        break;
    }
}

void UpdateState::exit()
{
    log.info("UpdateState", "Exiting UpdateState");
}

bool UpdateState::checkUpdateConditions()
{
    uint32_t now = millis();
    RuntimeConfig &config = configManager.getRuntimeConfig();

    if (lastUpdateCheck == 0 || (now - lastUpdateCheck) >= config.update.interval)
    {
        lastUpdateCheck = now;
        return true;
    }

    return false;
}

bool UpdateState::checkLatestRelease()
{
    HTTPClient http;
    RuntimeConfig &config = configManager.getRuntimeConfig();

    http.begin(config.update.apiUrl);
    http.addHeader("Accept", "application/vnd.github+json");
    http.addHeader("Authorization", String("Bearer ") + config.update.apiToken);
    http.addHeader("X-GitHub-Api-Version", "2022-11-28");
    http.addHeader("User-Agent", "ESP32");

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_NOT_FOUND)
    {
        reportProgress("No releases found");
        return false;
    }

    if (httpCode != HTTP_CODE_OK)
    {
        char msgBuffer[64];
        snprintf(msgBuffer, sizeof(msgBuffer), "Failed to check for updates: %d", httpCode);
        reportProgress(msgBuffer);
        return false;
    }

    String response = http.getString();
    if (response.length() == 0)
    {
        reportProgress("Empty response from update server");
        return false;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
        reportProgress("Failed to parse update info");
        return false;
    }

    const char *tagName = doc["tag_name"];
    if (!tagName)
    {
        reportProgress("No version tag found");
        return false;
    }

    bool foundFirmware = false;
    JsonArray assets = doc["assets"];
    for (JsonVariant asset : assets)
    {
        const char *name = asset["name"];
        if (strcmp(name, "firmware.bin") == 0)
        {
            downloadUrl = asset["browser_download_url"].as<String>();
            foundFirmware = true;
            break;
        }
    }

    if (!foundFirmware)
    {
        reportProgress("No firmware found in release");
        return false;
    }

    if (compareVersion(VERSION_STRING, tagName))
    {
        newVersion = tagName;
        char msgBuffer[128];
        snprintf(msgBuffer, sizeof(msgBuffer), "New version available: %s", tagName);
        reportProgress(msgBuffer, 0);
        return true;
    }

    char msgBuffer[128];
    snprintf(msgBuffer, sizeof(msgBuffer), "Current version up to date (%s)", VERSION_STRING);
    reportProgress(msgBuffer);
    return false;
}

bool UpdateState::downloadAndInstall()
{
    if (downloadUrl.length() == 0)
    {
        currentPhase = UpdatePhase::FAILED;
        handleUpdateError("No download URL available");

        return false;
    }

    HTTPClient http;

    http.begin(downloadUrl);

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK)
    {
        totalBytes = http.getSize();
        WiFiClient *stream = http.getStreamPtr();

        if (!Update.begin(totalBytes))
        {
            currentPhase = UpdatePhase::FAILED;
            handleUpdateError("Update could not begin");

            return false;
        }

        size_t written = 0;
        uint8_t buffer[128] = {0};

        while (http.connected() && (written < totalBytes))
        {
            size_t streamLength = stream->available();
            if (streamLength > 0)
            {
                size_t bytesRemaining = streamLength > size_t(sizeof(buffer)) ? sizeof(buffer) : streamLength;
                size_t bytesRead = stream->readBytes(buffer, bytesRemaining);

                if (Update.write(buffer, bytesRead) != bytesRead)
                {
                    currentPhase = UpdatePhase::FAILED;
                    handleUpdateError("Error writing to flash");

                    return false;
                }

                written += bytesRead;
                int progress = (written * 100) / totalBytes;
                reportProgress("Downloading", progress);
            }
            delay(1);
        }

        if (written == totalBytes && Update.end(true))
        {
            reportProgress("Download and installation completed", 100);

            return true;
        }
    }

    http.end();
    Update.abort();
    currentPhase = UpdatePhase::FAILED;

    return false;
}

bool UpdateState::compareVersion(const char *versionA, const char *versionB)
{
    if (newVersion[0] == 'v')
    {
        versionB++;
    }

    if (versionA[0] == 'v')
    {
        versionA++;
    }

    int currentVersion[3], newVersion[3];
    sscanf(versionA, "%d.%d.%d", &currentVersion[0], &currentVersion[1], &currentVersion[2]);
    sscanf(versionB, "%d.%d.%d", &newVersion[0], &newVersion[1], &newVersion[2]);

    for (int i = 0; i < 3; i++)
    {
        if (newVersion[i] > currentVersion[i])
        {
            return true;
        }

        if (newVersion[i] < currentVersion[i])
        {
            return false;
        }
    }

    return false;
}

void UpdateState::reportProgress(const char *status, int progress)
{
    if (mqttManager.isConnected())
    {
        StaticJsonDocument<256> doc;
        doc["status"] = status;

        if (progress >= 0)
        {
            doc["progress"] = progress;
        }
        if (newVersion.length() > 0)
        {
            doc["version"] = newVersion;
        }

        String topic = String(mqttManager.getDeviceTopic()) + "/update";
        String payload;
        serializeJson(doc, payload);

        mqttManager.publish(topic.c_str(), payload.c_str());
    }

    if (progress >= 0)
    {
        char msgBuffer[128];
        snprintf(msgBuffer, sizeof(msgBuffer), "%s (%d%%)", status, progress);
        log.info("UpdateState", msgBuffer);
    }
    else
    {
        log.info("UpdateState", status);
    }
}

void UpdateState::handleUpdateError(const char *message)
{
    ErrorState::getInstance(device).setError(
        ErrorCode::UPDATE_FAILED,
        this,
        message);
}