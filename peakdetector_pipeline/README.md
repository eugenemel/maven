# How to use peakdetector in a custom pipeline

`peakdetector` is a command line program based on the `maven_core` library for performing peak picking, grouping, and spectral library searches.  The program is published in the [releases](https://github.com/eugenemel/maven/releases) section of this repository alongside the MAVEN GUI.

It is designed to be incorporated into data science pipelines written in `R` or `Python`. 

See [peakdetector-in-pipeline-example-mac-osx.Rmd](https://github.com/eugenemel/maven/blob/master/peakdetector_pipeline/peakdetector-in-pipeline-example-mac-osx.Rmd) for an example of how to incorporate `peakdetector` into your mass spectrometry pipeline.  

Currently, only `mac os x` is supported. The above example is written for an `R` pipeline, but the generated command line will also work for a `Python` pipeline.