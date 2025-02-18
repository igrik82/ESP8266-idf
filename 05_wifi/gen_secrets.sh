#!/bin/bash

cat <<EOF >main/secrets.h
#pragma once
#define WIFI_SSID "$WIFI_SSID"
#define WIFI_PASSWORD "$WIFI_PASSWORD"

EOF

echo "Updated secret.h with WIFI_SSID=$WIFI_SSID and WIFI_PASSWORD=$WIFI_PASSWORD"
