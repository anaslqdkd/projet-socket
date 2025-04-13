#!/bin/bash

# === Configuration ===
CERT_DIR="certs"
COMMON_NAME="Orchestrator"
CA_CERT="ca/ca-cert.pem"         # Must already exist in this VM
CA_KEY="ca/ca-key.pem"           # Must already exist in this VM (or brought temporarily)

# === Ensure cert directory ===
mkdir -p "$CERT_DIR"

# === Generate private key ===
openssl genrsa -out "$CERT_DIR/orchestrator-key.pem" 2048

# === Create CSR ===
openssl req -new -key "$CERT_DIR/orchestrator-key.pem" \
    -out "$CERT_DIR/orchestrator.csr" -subj "/CN=$COMMON_NAME"

# === Sign certificate with CA ===
openssl x509 -req \
    -in "$CERT_DIR/orchestrator.csr" \
    -CA "$CA_CERT" -CAkey "$CA_KEY" -CAcreateserial \
    -out "$CERT_DIR/orchestrator-cert.pem" -days 365 -sha256

# === Clean up CSR ===
rm "$CERT_DIR/orchestrator.csr"

echo "[+] Certificat de l'orchestrateur généré dans $CERT_DIR"
