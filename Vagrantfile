# -*- mode: ruby -*-
# vi: set ft=ruby ts=2 sts=2 sw=2 et :

# All Vagrant configuration is done below. The "2" in Vagrant.configure
# configures the configuration version (we support older styles for
# backwards compatibility). Please don't change it unless you know what
# you're doing.
Vagrant.configure("2") do |config|
  # Every Vagrant development environment requires a box. You can search for
  # boxes at https://atlas.hashicorp.com/search.
  config.vm.box = "ubuntu/trusty64"

  config.vm.provider "lxc" do |lxc, override|
    override.vm.box = "fgrehm/trusty64-lxc"
  end


  #
  # mass-define "generic" Ubuntu boxes
  #
  %w{5.1 5.2 5.4 5.7 5.8 5.9}.each { |version|
    config.vm.define "v8-#{version}" do |i|
      i.vm.synced_folder ".", "/data/v8js"

      i.vm.provision "shell", inline: <<-SHELL
      gpg --keyserver keys.gnupg.net --recv 7F438280EF8D349F
      gpg --armor --export 7F438280EF8D349F | apt-key add -

      apt-get update
      apt-get install -y software-properties-common gdb tmux git tig curl apache2-utils

      add-apt-repository ppa:ondrej/php
      add-apt-repository ppa:pinepain/libv8-#{version}
      apt-get update
      apt-get install -y php7.1-dev libv8-#{version}-dbg libv8-#{version}-dev
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


  config.vm.provision "shell", inline: <<-SHELL
    mkdir -p /data/build && chown vagrant:vagrant /data/build
  SHELL
end
