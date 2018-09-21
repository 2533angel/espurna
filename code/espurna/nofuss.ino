/*

NOFUSS MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

Module key prefix: nof

*/

#if NOFUSS_SUPPORT

#include "NoFUSSClient.h"

unsigned long _nofussLastCheck = 0;
unsigned long _nofussInterval = 0;
bool _nofussEnabled = false;

// -----------------------------------------------------------------------------
// NOFUSS
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

void _nofussWebSocketOnSend(JsonObject& root) {
    root["nofVisible"] = 1;
    root["nofEnabled"] = getSetting("nofEnabled", NOFUSS_ENABLED).toInt() == 1;
    root["nofServer"] = getSetting("nofServer", NOFUSS_SERVER);
}

#endif // WEB_SUPPORT

void _nofussConfigure() {

    String nofussServer = getSetting("nofServer", NOFUSS_SERVER);
    #if MDNS_CLIENT_SUPPORT
        nofussServer = mdnsResolve(nofussServer);
    #endif

    if (nofussServer.length() == 0) {
        setSetting("nofEnabled", 0);
        _nofussEnabled = false;
    } else {
        _nofussEnabled = getSetting("nofEnabled", NOFUSS_ENABLED).toInt() == 1;
    }
    _nofussInterval = getSetting("nofInterval", NOFUSS_INTERVAL).toInt();
    _nofussLastCheck = 0;

    if (!_nofussEnabled) {

        DEBUG_MSG_P(PSTR("[NOFUSS] Disabled\n"));

    } else {

        char buffer[20];
        snprintf_P(buffer, sizeof(buffer), PSTR("%s-%s"), APP_NAME, getDevice().c_str());

        NoFUSSClient.setServer(nofussServer);
        NoFUSSClient.setDevice(buffer);
        NoFUSSClient.setVersion(APP_VERSION);

        DEBUG_MSG_P(PSTR("[NOFUSS] Server : %s\n"), nofussServer.c_str());
        DEBUG_MSG_P(PSTR("[NOFUSS] Dervice: %s\n"), buffer);
        DEBUG_MSG_P(PSTR("[NOFUSS] Version: %s\n"), APP_VERSION);
        DEBUG_MSG_P(PSTR("[NOFUSS] Enabled\n"));

    }

}

bool _nofussKeyCheck(const char * key) {
    return (strncmp(key, "nof", 3) == 0);
}

void _nofussBackwards() {
    moveSettings("nofussServer", "nofServer"); // 1.14.0  2018-06-26
    moveSettings("nofussEnabled", "nofEnabled"); // 1.14.0  2018-06-26
    moveSettings("nofussInterval", "nofInterval"); // 1.14.0  2018-06-26
}

#if TERMINAL_SUPPORT

void _nofussInitCommands() {

    settingsRegisterCommand(F("NOFUSS"), [](Embedis* e) {
        DEBUG_MSG_P(PSTR("+OK\n"));
        nofussRun();
    });

}

#endif // TERMINAL_SUPPORT

// -----------------------------------------------------------------------------

void nofussRun() {
    NoFUSSClient.handle();
    _nofussLastCheck = millis();
}

void nofussSetup() {

    _nofussBackwards();
    _nofussConfigure();

    NoFUSSClient.onMessage([](nofuss_t code) {

        if (code == NOFUSS_START) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Start\n"));
        }

        if (code == NOFUSS_UPTODATE) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Already in the last version\n"));
        }

        if (code == NOFUSS_NO_RESPONSE_ERROR) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Wrong server response: %d %s\n"), NoFUSSClient.getErrorNumber(), (char *) NoFUSSClient.getErrorString().c_str());
        }

        if (code == NOFUSS_PARSE_ERROR) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Error parsing server response\n"));
        }

        if (code == NOFUSS_UPDATING) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Updating\n"));
    	    DEBUG_MSG_P(PSTR("         New version: %s\n"), (char *) NoFUSSClient.getNewVersion().c_str());
        	DEBUG_MSG_P(PSTR("         Firmware: %s\n"), (char *) NoFUSSClient.getNewFirmware().c_str());
        	DEBUG_MSG_P(PSTR("         File System: %s\n"), (char *) NoFUSSClient.getNewFileSystem().c_str());
            #if WEB_SUPPORT
                wsSend_P(PSTR("{\"message\": 1}"));
            #endif
        }

        if (code == NOFUSS_FILESYSTEM_UPDATE_ERROR) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] File System Update Error: %s\n"), (char *) NoFUSSClient.getErrorString().c_str());
        }

        if (code == NOFUSS_FILESYSTEM_UPDATED) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] File System Updated\n"));
        }

        if (code == NOFUSS_FIRMWARE_UPDATE_ERROR) {
            DEBUG_MSG_P(PSTR("[NoFUSS] Firmware Update Error: %s\n"), (char *) NoFUSSClient.getErrorString().c_str());
        }

        if (code == NOFUSS_FIRMWARE_UPDATED) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Firmware Updated\n"));
        }

        if (code == NOFUSS_RESET) {
        	DEBUG_MSG_P(PSTR("[NoFUSS] Resetting board\n"));
            #if WEB_SUPPORT
                wsSend_P(PSTR("{\"action\": \"reload\"}"));
            #endif
            nice_delay(100);
        }

        if (code == NOFUSS_END) {
    	    DEBUG_MSG_P(PSTR("[NoFUSS] End\n"));
        }

    });

    #if WEB_SUPPORT
        wsOnSendRegister(_nofussWebSocketOnSend);
    #endif

    #if TERMINAL_SUPPORT
        _nofussInitCommands();
    #endif

    settingsRegisterKeyCheck(_nofussKeyCheck);

    // Main callbacks
    espurnaRegisterLoop(nofussLoop);
    espurnaRegisterReload(_nofussConfigure);

}

void nofussLoop() {

    if (!_nofussEnabled) return;
    if (!wifiConnected()) return;
    if ((_nofussLastCheck > 0) && ((millis() - _nofussLastCheck) < _nofussInterval)) return;

    nofussRun();

}

#endif // NOFUSS_SUPPORT
