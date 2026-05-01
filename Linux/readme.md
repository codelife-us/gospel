You have to install python3-pip and curl support on Linux. These commands will work on Ubuntu:
sudo apt install python3-pip
sudo apt install curl
sudo apt install python3-tk
python3 -m pip install --break-system-packages pillow

If you already installed Pillow before python3-tk, reinstall it so ImageTk support is included:
python3 -m pip install --force-reinstall --break-system-packages pillow

sudo apt install imagemagick

Make bvilive executable:
chmod +x bvilive
