Simple Compressor plugin written using JUCE framework.

The basic signal flow is:
EQ -> Compressor -> Soft Clipper
- EQ allows you to boost/cut certain frequencies to be compressed more/less
- The compressor is... a normal compressor
- The soft clipper prevents harsh clipping of any signal above full scale

If you would like to build this plugin, download the JUCE framework: https://juce.com/download/
Included in this repo are the required projucer file and source files needed to build

These tutorials will help you with using Projucer:
https://docs.juce.com/master/tutorial_new_projucer_project.html
https://docs.juce.com/master/tutorial_manage_projucer_project.html
