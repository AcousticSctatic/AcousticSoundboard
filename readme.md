![AcousticSoundboard_2022-01-23_12-15-54](https://user-images.githubusercontent.com/64415017/150692162-4ae9ae60-7821-4e36-9b6a-6630eb629706.png)

Acoustic Soundboard is a program for Windows which strives to be easy to use and lightweight. The main purpose is to conveniently play audio files to others while allowing the user to hear what is playing and still use their microphone normally. There is no installation required, simply unzip the contents wherever you like. On closing, the soundboard will automatically save the user's last devices, sounds, and hotkeys to a database file.

If you want others to hear your microphone and the sounds you play, you will first need to install a virtual audio cable such as this one:
https://vb-audio.com/Cable/

If you have suggestions or questions, message me on [discord](https://discord.gg/u65C78qzKs)
----------------------------------------
Instructions
* Download the zip file under releases (or clone the repo and compile it yourself) and extract to your folder of choice. https://github.com/AcousticSctatic/AcousticSoundboard/releases/tag/BETA
* Launch AcousticSoundboard and click "Select Playback Device" and select: 
  1. the device where YOU want to hear the sound
  2. the virtual device (eg virtual audio cable input)
* Next, click "Select Recording Device" and select your microphone. Then select the virtual device from the previous step. This is the device where your microphone's audio will be sent.
* Click "Select file" and select a sound file. Most common file formats are supported.
* Click "Set hotkey" and set a hotkey that will trigger the sound. You can use SHIFT, CONTROL, or ALT key combos, and you can set up to 50 hotkeys and sounds.
* Press your hotkey. Have fun!

With this setup, you should be able to select the virtual audio cable as your microphone in your voice call program of choice, and others should hear your voice through your microphone and the sounds you play. When you are finished, close the window and your settings will be saved automatically. Do not close your devices first, or they will be forgotten the next time you run the soundboard. 

Notes
* You can press the PAUSE|BREAK key on your keyboard to stop all sounds
* Num pad keys work, but they do not display as unique text (yet).
----------------------------------------
Help section
* IMPORTANT: Windows may warn you that this program contains a virus because it hooks the keyboard. Technically, it is a keyloger, but not the malicious kind. Your data stays on your computer only. See the next note below.
* IMPORTANT: If your hotkeys do not work inside another program, try running Acoustic Soundboard as administrator. This is due to a Windows security feature (integrity levels). In general, you should avoid running programs as administrator, but this one is open source, so you can verify that it is safe to use and compile it yourself.
* The soundboard only knows about devices that were detected when it started. In other words, if you plug in a microphone or speakers while the sondboard is running, you need to close and restart the soundboard for it to detect them.
* If your program does not allow you to select a microphone, you may have to set the virtual audio cable as your default recording (input) device in Windows before starting your other program.
* If you want to completely reset all settings, you can simply delete the "hotkeys.db" file the soundboard creates, and you will have a blank slate the next time you start it. You should probably close the program before doing so, though.
 ----------------------------------------
