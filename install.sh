#!/bin/bash

dist_name=$(grep DISTRIB_ID /etc/lsb-release | cut -d= -f2 | tr -d '"')

if [ "$dist_name" == "LinuxMint" ]; then
    sudo apt install -y make gcc pkg-config neovim openssl

elif [ "$dist_name" == "ManjaroLinux" ]; then
	  if ! pacman -Qs make &> /dev/null; then
		  echo "installing make"

        sudo pacman -S --noconfirm make
    fi
    if ! pacman -Qs gcc &> /dev/null; then
		  echo "installing gcc"
        sudo pacman -S --noconfirm gcc
    fi
    if ! pacman -Qs pkgconf &> /dev/null; then
		  echo "installing pkgconf"
        sudo pacman -S --noconfirm pkgconf
    fi
    if ! pacman -Qs neovim &> /dev/null; then
		  echo "installing neovim"
        sudo pacman -S --noconfirm neovim
    fi
    if ! pacman -Qs openssl &> /dev/null; then
		  echo "installing openssl"
        sudo pacman -S --noconfirm openssl
    fi
else
    echo "Unsupported distribution."
fi

rm -rf ca
rm -rf certs

echo "generating certificate"
./src/generate_ca.sh
./src/generate_orchestrator_cert.sh
./src/generate_agent_cert.sh

