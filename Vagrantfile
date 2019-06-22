# -*- mode: ruby -*-
# vi: set ft=ruby ts=2 sts=2 sw=2 et :

# All Vagrant configuration is done below. The "2" in Vagrant.configure
# configures the configuration version (we support older styles for
# backwards compatibility). Please don't change it unless you know what
# you're doing.
Vagrant.configure("2") do |config|
  # Every Vagrant development environment requires a box. You can search for
  # boxes at https://atlas.hashicorp.com/search.
  config.vm.box = "ubuntu/bionic64"


  #
  # mass-define "generic" Ubuntu boxes
  #
  %w{7.5}.each { |version|
    config.vm.define "v8-#{version}" do |i|
      i.vm.synced_folder ".", "/data/v8js"

      i.vm.provision "shell", inline: <<-SHELL
      gpg --keyserver keys.gnupg.net --recv 7F438280EF8D349F
      gpg --armor --export 7F438280EF8D349F | apt-key add -

      apt-get update
      apt-get install -y software-properties-common gdb tmux git tig curl apache2-utils lcov

      add-apt-repository ppa:ondrej/php
      add-apt-repository ppa:stesie/libv8
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

  config.vm.provision "shell", privileged: false, inline: <<-SHELL
    sudo mkdir -p /data/build && sudo chown $USER:$USER /data/build
  SHELL
end
