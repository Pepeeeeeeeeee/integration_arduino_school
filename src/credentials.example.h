#pragma once
// Copy this file to credentials.h (same folder) and fill in real values.
// credentials.h is gitignored and never committed - see README.MD >
// Installation for details.

static String WIFI_SSID  = "your-network-name";
static String WIFI_PASS  = "your-network-password";
static String API_URL    = "http://<collector-host>:8000/v1/ingest"; // data collector service
static String API_KEY    = "your-collector-api-key"; // shared secret with the data collector service
static String DEVICE_UID = "test"; // device name sent with each measurement
