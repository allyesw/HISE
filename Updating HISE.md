# Updating HISE


### Develop Branch - WITH FAUST

1. Sync the latest changes from the original HISE to your fork ("develop-mac" for Mac, "develop-win" for Win).
You have to use Terminal for this. Go to GitHub Desktop > Repository > Open in Terminal. Then:
	a. Check that christophhart/HISE is set as the upstream repo:
		`git remote show upstream`
		(It should be, but if not, use `git remote add upstream https://github.com/christophhart/HISE.git`)
	b. `git fetch upstream`. Nothing will happen.
	c. `git checkout develop-mac` (unless on PC). It should say that's already where you are.
	d. `git merge upstream/develop`. You'll probably get a lot of errors. Open GitHub Desktop and you should be prompted to resolve the conflicts. Choose "use the modified file from develop" (the upstream one) for everything but files you've modified. For those, diff the files and edit as needed.

	If it's been a long time, or if files you've modified have conflicts, this might not work. In that case, you'll just have to manually redo all your changes. Here are all the changes I've made so far:
		- Removed green SampleStartMod color (hi_core/hi_components/audio_components/SampleComponents.cpp)
		- Changed panel default properties (hi_scripting/scripting/api/ScriptingApiContent.cpp)
		- Changed dragged out MIDI filename (hi_core/hi_dsp/modules/MidiPlayer.cpp)
		- Changed wavetable waterfall colors (hi_core/hi_components/audio_components/SampleComponents.cpp)
		- Fixed MidiOverlay DAW crash on second instance (hi_core/hi_components/midi_overlays/MidiOverlayFactory.h). *NOTE: This needs to be tested in exported plugins because I'm not sure that removing the DeletedAtShutdown class won't screw other things up. More info: https://github.com/allyesw/HISE/commit/49028f65941185b389645d927b18610d3b8e5f31*

	If you have to do this manually, make sure to preserve this file and continue on to make the other changes below.



2. Copy the contents of `/tools/SDK` and `/tools/faust/` from the old folder to the new one.

3. Open `/projects/standalone/HISE Standalone.jucer`.

4. ON MAC:
	In the `hi_core` module, make sure the `USE_IPP` flag is set to *FALSE*.
	In the `Xcode (macOS)` exporter tab, add the following to Extra Preprocessor Definitions:
	`NUM_HARDCODED_FX_MODS=6 NUM_HARDCODED_POLY_FX_MODS=6`
	In the `Xcode (macOS)` exporter tab, scroll to `Valid Architectures`, and make sure only `x86_64` is checked.

   ON WIN:
   	In the `hi_core` module, make sure the `USE_IPP` flag is set to *TRUE*.
   	In the `Visual Studio 2022` exporter tab, add `NUM_HARDCODED_FX_MODS=6` and `NUM_HARDCODED_POLY_FX_MODS=6` to Extra Preprocessor Definitions.
   	In the `Release with Faust` tab, add "C:\Program Files (x86)\Intel\oneAPI\ipp\2021.10\include" to Header Search Paths. (NOTE: You'll have to do that when exporting plugins too.)

   ON BOTH:
   	In the `hi_faust` module, make sure `HISE_INCLUDE_FAUST` is enabled.
   	In the `hi_faust_types` module, if you're using a faust version older than 2.54.0 (which I am NOT as of 1/16/25. Now on 2.77.3), also enable `FAUST_NO_WARNING_MESSAGES`.

5. Click `Save and Open in IDE`.

6. ON MAC:
	Click Product > Scheme > Edit Scheme.
	Choose `Release with Faust`. Make sure `Debug Executable` is checked.
	Click Product > Build For > Running.

   ON WIN:
   	Choose `Release with Faust` in the dropdown in the top bar.
   	Click Build > Build Solution.



*NOTE* If you need to compile HISE with Faust as a plugin, the plugin Projucer file is not set up correctly for that. Open the standalone Projucer file and add all the Faust-related settings, flags, etc. to the plugin version. Then in Xcode, choose Product > Scheme > `HISE - All`, then Product > Build For > Profiling.




