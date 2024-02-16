Build the project
======================
1. Install esp-idf 5.1.2 https://docs.espressif.com/projects/esp-idf/en/v5.1.2/esp32/get-started/index.html
2. Intsall Git
3. Clone the project:
```
git clone --recurse-submodules https://github.com/szevczenko/esp32_valves_regulator.git
```
4. Run ESP-IDF 5.1 CMD and enter to cloned folder.
5. build by running command:
```
idf.py build
```
6. flash esp32:
```
idf.py flash -p COM8
```
Test web UI
======================
1. Connect to WiFi access point VALVE by computer or phone. Default password:
```
SuperTrudne1!-_
```
2. Write in browser:
```
http://192.168.4.1
```
3. Default caredential:

| Username  | Password |
| --------- | -------- |
|   admin   |   admin  |
|   user1   |   user1  |
|   user2   |   user2  |
