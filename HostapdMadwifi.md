# Introduction #

  * read the README file.
  * copy defconfig to .config.Then change the configuration
```
CONFIG_DRIVER_MADWIFI=y
CFLAGS += -I../madwifi-0.9.4 # change to reflect local setup; directory for madwifi src
```

  * install libssl-dev. apt-get install libssl-dev