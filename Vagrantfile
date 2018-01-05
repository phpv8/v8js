# -*- mode: ruby -*-
# vi: set ft=ruby ts=2 sts=2 sw=2 et :

# All Vagrant configuration is done below. The "2" in Vagrant.configure
# configures the configuration version (we support older styles for
# backwards compatibility). Please don't change it unless you know what
# you're doing.
Vagrant.configure("2") do |config|
  # Every Vagrant development environment requires a box. You can search for
  # boxes at https://atlas.hashicorp.com/search.
  config.vm.box = "ubuntu/xenial64"

  config.vm.provider "lxc" do |lxc, override|
    lxc.backingstore = "none"
    override.vm.box = "zaikin/xenial64-lxc"
  end


  #
  # mass-define "generic" Ubuntu boxes
  #
  %w{6.3 6.4 6.5}.each { |version|
    config.vm.define "v8-#{version}" do |i|
      i.vm.synced_folder ".", "/data/v8js"

      i.vm.provision "shell", inline: <<-SHELL
      gpg --keyserver keys.gnupg.net --recv 7F438280EF8D349F
      gpg --armor --export 7F438280EF8D349F | apt-key add -

      apt-get update
      apt-get install -y software-properties-common gdb tmux git tig curl apache2-utils lcov

      add-apt-repository ppa:ondrej/php
      add-apt-repository ppa:pinepain/libv8-#{version}
      apt-get update
      apt-get install -y php7.1-dev libv8-#{version}-dbg libv8-#{version}-dev
    SHELL
    end
  }

  %w{}.each { |version|
    config.vm.define "v8-#{version}" do |i|
      i.vm.synced_folder ".", "/data/v8js"

      i.vm.provision "shell", inline: <<-SHELL
      gpg --keyserver keys.gnupg.net --recv 7F438280EF8D349F
      gpg --armor --export 7F438280EF8D349F | apt-key add -

      apt-get update
      apt-get install -y software-properties-common gdb tmux git tig curl apache2-utils lcov build-essential python libglib2.0-dev

      add-apt-repository ppa:ondrej/php
      apt-get update
      apt-get install -y php7.1-dev

      mkdir -p /data
      cd /data

      # Install depot_tools first (needed for source checkout)
      git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
      export PATH=`pwd`/depot_tools:"$PATH"

      # Download v8
      fetch v8
      cd v8

      # checkout version
      git checkout #{version}
      gclient sync

      # Setup GN
      tools/dev/v8gen.py -vv x64.release -- is_component_build=true

      # Build
      ninja -C out.gn/x64.release/

      # Install to /opt/libv8-#{version}/
      sudo mkdir -p /opt/libv8-#{version}/{lib,include}
      sudo cp out.gn/x64.release/lib*.so out.gn/x64.release/*_blob.bin out.gn/x64.release/icudtl.dat /opt/libv8-#{version}/lib/
      sudo cp -R include/* /opt/libv8-#{version}/include/
    SHELL
    end
  }


  #
  # Fedora-based box with GCC7, V8 5.2 + PHP 7.1 installed
  # (primarily to reproduce #294)
  #
  config.vm.define "fedora-26-gcc7" do |i|
    i.vm.box = "vbenes/fedora-rawhide-server"
    i.ssh.insert_key = false

    # unfortunately vboxsf isn't currently available (due to an issue with the base image)
    # therefore fall back to nfs
    i.vm.synced_folder ".", "/data/v8js", type: "nfs"
    i.vm.network "private_network", ip: "192.168.50.2"

    i.vm.provision "shell", inline: <<-SHELL
      dnf -y update
      dnf -y install gcc-c++ gdb tmux git tig curl vim
      dnf -y install v8-devel php-devel
    SHELL
  end


  #
  # FreeBSD 11.0 box
  # (compiles V8 5.1.281.47 with Gyp; using port from https://raw.githubusercontent.com/Kronuz/Xapiand/master/contrib/freebsd/v8.shar)
  #
  config.vm.define "freebsd-11" do |i|
    i.vm.box = "freebsd/FreeBSD-11.0-RELEASE-p1"
    i.ssh.shell = "/bin/sh"

    # vboxsf doesn't work on FreeBSD (yet), use nfs
    i.vm.synced_folder ".", "/data/v8js", type: "nfs"
    i.vm.network "private_network", type: "dhcp"

    i.vm.provision "shell", inline: <<-SHELL
      pkg install -y git python bash gmake icu gdb tmux git tig curl vim autoconf php70

      portsnap auto --interactive

      mkdir -p /data && cd /data
      [ -x v8 ] || curl https://raw.githubusercontent.com/Kronuz/Xapiand/master/contrib/freebsd/v8.shar | sh

      cd /data/v8
      make install
    SHELL
  end


  #
  # Fedora 25 box with 32-bit
  #
  config.vm.define "fedora25-32" do |i|
    i.vm.box = "wholebits/fedora25-32"
    i.vm.synced_folder ".", "/data/v8js"

    i.vm.provision "shell", inline: <<-SHELL
      dnf -y update
      dnf -y install gcc-c++ gdb tmux git tig curl vim
      dnf -y install v8-devel php-devel
    SHELL
  end

  config.vm.define "macos-sierra" do |i|
    i.vm.box = "gobadiah/macos-sierra"

    i.vm.synced_folder ".", "/data/v8js", type: "nfs", mount_options:["resvport"]
    i.vm.network "private_network", ip: "192.168.50.3"

    i.vm.provider "virtualbox" do |vb|
      vb.memory = "3000"

      vb.customize ["modifyvm", :id, "--cpuidset", "1","000106e5","00100800","0098e3fd","bfebfbff"]
      vb.customize ["setextradata", :id, "VBoxInternal/Devices/efi/0/Config/DmiSystemProduct", "iMac11,3"]
      vb.customize ["setextradata", :id, "VBoxInternal/Devices/efi/0/Config/DmiSystemVersion", "1.0"]
      vb.customize ["setextradata", :id, "VBoxInternal/Devices/efi/0/Config/DmiBoardProduct", "Iloveapple"]
      vb.customize ["setextradata", :id, "VBoxInternal/Devices/smc/0/Config/DeviceKey", "ourhardworkbythesewordsguardedpleasedontsteal(c)AppleComputerInc"]
      vb.customize ["setextradata", :id, "VBoxInternal/Devices/smc/0/Config/GetKeyFromRealSMC", 1]
    end

    i.vm.provision "shell", privileged: false, inline: <<-SHELL
      # install homebrew
      /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
      brew install autoconf
      brew install homebrew/php/php71
      brew install v8
    SHELL
  end

  config.vm.provision "shell", privileged: false, inline: <<-SHELL
    sudo mkdir -p /data/build && sudo chown $USER:$USER /data/build
  SHELL
end
