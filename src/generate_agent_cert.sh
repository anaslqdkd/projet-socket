#!/bin/bash

# === Configuration ===
CERT_DIR="certs"
COMMON_NAME="Agent"
CA_CERT="ca/ca-cert.pem"         # Must already exist in this VM
CA_KEY="ca/ca-key.pem"           # Must already exist in this VM (or brought temporarily)

# === Ensure cert directory ===
mkdir -p "$CERT_DIR"

# === Generate private key ===
openssl genrsa -out "$CERT_DIR/agent-key.pem" 2048

# === Create CSR ===
openssl req -new -key "$CERT_DIR/agent-key.pem" \
    -out "$CERT_DIR/agent.csr" -subj "/CN=$COMMON_NAME"

# === Sign certificate with CA ===
openssl x509 -req \
    -in "$CERT_DIR/agent.csr" \
    -CA "$CA_CERT" -CAkey "$CA_KEY" -CAcreateserial \
    -out "$CERT_DIR/agent-cert.pem" -days 365 -sha256

# === Clean up CSR ===
rm "$CERT_DIR/agent.csr"

echo "[+] Certificat d'agent généré dans $CERT_DIR"
