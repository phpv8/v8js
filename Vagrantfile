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

  # Share an additional folder to the guest VM. The first argument is
  # the path on the host to the actual folder. The second argument is
  # the path on the guest to mount the folder. And the optional third
  # argument is a set of non-required options.
  config.vm.synced_folder ".", "/data/v8js"

  config.vm.provision "shell", inline: <<-SHELL
    gpg --keyserver keys.gnupg.net --recv 7F438280EF8D349F
    gpg --armor --export 7F438280EF8D349F | apt-key add -

    apt-get update
    apt-get install -y software-properties-common gdb tmux git tig curl apache2-utils
    add-apt-repository ppa:ondrej/php
  SHELL

  config.vm.define "v8-5.8" do |i|
    i.vm.provision "shell", inline: <<-SHELL
    add-apt-repository ppa:pinepain/libv8-5.8
    apt-get update
    apt-get install -y php7.0-dev libv8-5.8-dbg libv8-5.8-dev
  SHELL
  end

  config.vm.define "v8-5.7" do |i|
    i.vm.provision "shell", inline: <<-SHELL
    add-apt-repository ppa:pinepain/libv8-5.7
    apt-get update
    apt-get install -y php7.0-dev libv8-5.7-dbg libv8-5.7-dev
  SHELL
  end

  config.vm.define "v8-5.2" do |i|
    i.vm.provision "shell", inline: <<-SHELL
    add-apt-repository ppa:pinepain/libv8-5.2
    apt-get update
    apt-get install -y php7.0-dev libv8-5.2-dbg libv8-5.2-dev
  SHELL
  end
end
