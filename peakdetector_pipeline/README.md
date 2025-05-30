# How to use peakdetector in a custom pipeline

`peakdetector` is a command line program based on the `maven_core` library for performing peak picking, grouping, and spectral library searches.  The program is published in the [releases](https://github.com/eugenemel/maven/releases) section of this repository alongside the MAVEN GUI.

It is designed to be incorporated into data science pipelines written in `R` or `Python`. 

The examples below are designed to work in `R`, but something similar could be developed for `Python` as well. The key step is the command line that is generated, which should be executed via a system call.

# Windows

[peakdetector-in-pipeline-example-windows.Rmd](https://github.com/eugenemel/maven/blob/master/peakdetector_pipeline/peakdetector-in-pipeline-example-windows.Rmd)

# Mac OS
 [peakdetector-in-pipeline-example-mac-osx.Rmd](https://github.com/eugenemel/maven/blob/master/peakdetector_pipeline/peakdetector-in-pipeline-example-mac-osx.Rmd)