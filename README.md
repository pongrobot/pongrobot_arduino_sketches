# pongrobot-arduino-sketches

This repository holds prototype Arduino sketches to evaluate sensor breakouts, motor drivers, and other components. These sketches generally aren't used in the real robot -- Those are included with the git repositories for the respective ROS packages.

## Installing udev rules.

For the software to properly identify hardware devices, udev rules should be installed. There are two udev rules for the Arduinos. Install them by creating symlinks:
```
sudo ln -s /home/bro/pongrobot_arduino_sketches/udev/10-brobot-launcher.rules /etc/udev/rules.d/10-brobot-launcher.rules
sudo ln -s /home/bro/pongrobot_arduino_sketches/udev/11-brobot-tf.rules /etc/udev/rules.d/11-brobot-tf.rules
```
