AcousticSoundboard is a Windows (Win32) program built with the DearImGui and miniaudio libraries. It began out of frustration with the available options for soundboards. It strives to be an easy to use, lightweight program that can play an audio file to others while allowing the user to hear what ia played and still use their microphone normally. To achieve this, you will first need to install a virtual audio cable such as this one:
https://vb-audio.com/Cable/

1. Once that's done, launch AcousticSoundboard and click "Select Playback Device" and select 
	a. the device where YOU want to hear the sound 
	b. the virtual device (eg virtual audio cable input)
2. Next, click "Select Recording Device" and select your microphone.
3. In the same window, select device b from step 1. This is the device where your microphone's audio will be sent.
4. Select a sound file and set a hotkey that will trigger the sound. You can set up to 50 sounds, and use SHIFT, CONTROL, or ALT key combos. Most common file formats are supported.

That's all!

With this setup, you should be able to select the virtual audio cable as your microphone in your voice call program of choice, and others should hear your voice through your microphone as well as the sounds you play. If the program does not allow you to select a microphone, you may have to set the virtual audio cable as your default recording (input) device in Windows.

NOTE: If your hotkeys do not work inside another program, try running AcousticSoundboard as administrator. This is due to a Windows security concept called integrity levels.
