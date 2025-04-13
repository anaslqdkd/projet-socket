#!/bin/bash

dist_name=$(grep DISTRIB_ID /etc/lsb-release | cut -d= -f2 | tr -d '"')

if [ "$dist_name" == "LinuxMint" ]; then
    sudo apt install -y make gcc pkg-config neovim openssl

elif [ "$dist_name" == "ManjaroLinux" ]; then
      sudo pacman -S gcc pkgconf neovim make openssl
else
    echo "Unsupported distribution."
fi

rm -rf ca
rm -rf certs

echo "generating certificate"
./src/generate_ca.sh
./src/generate_orchestrator_cert.sh
./src/generate_agent_cert.sh
make
