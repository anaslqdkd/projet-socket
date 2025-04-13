#!/bin/bash

# === Configuration ===
CA_DIR="ca"
CA_KEY="$CA_DIR/ca-key.pem"
CA_CERT="$CA_DIR/ca-cert.pem"

# === Ensure CA directory exists ===
mkdir -p "$CA_DIR"

# === Generate CA private key ===
openssl genrsa -out "$CA_KEY" 4096

# === Generate CA certificate (self-signed) ===
openssl req -x509 -new -nodes -key "$CA_KEY" \
    -sha256 -days 3650 \
    -out "$CA_CERT" \
    -subj "/CN=MyCustomCA"

echo "[+] Autorité de certification (CA) créée dans $CA_DIR"
