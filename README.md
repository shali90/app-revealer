# BOLOS app for Nano S using visual cryptography for seed backup

Nano S application to generate revealer
For information about revealer, please visit [revealer]

/!\ Developpment in progress, use at your own risk

# Setting up dev environnement
Please follow [instructions]

# Making/loading the app
Plug your Nano S, unlock it, go to dashboard
```sh
$ cd /bolos-app-revealer 
$ make all load
```

# Generating revealer
- Open the app on your Nano S
- Navigate to "Type your noise seed" menu
- Type your noise seed (noise seed is the number provided with your revealer card), use left/right button to switch digits, and both buttons to validate digit.
- Navigate to "Type your seed words" menu, choose the number of words, type your words

Once both noise seed and words are set, you can launch python revealer script
```sh
$ python revealer.py --apdu
```

Revealer image is updated row by row, 159 rows total, process is quite long (~3min)

[revealer]: <https://revealer.cc/>
[instructions]: <https://ledger.readthedocs.io/en/latest/userspace/getting_started.html>
