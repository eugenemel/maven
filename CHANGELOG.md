# Change Log
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [8.0.3][]
### Added
-   Implimentation of multivariate hypergeometic scoring for ms2 spectra @eugenemel
-   Maven can now read compressed mzML and mzXML samples with 32bit and 64bit encoding
-   Saving and loading of segmented alignments from mzrollDB @eugenemel
-   Loading and simulation search of multiple fragmentation libraries @eugenemel
-   Adjusted plot widgets to use bigger fonts @eugenemel
-   Implemented new scatter plot, m/z by rt of detected peakgroups

### Changed

### Deprecated

### Removed

### Fixed 
-   Fixed bug in hypergeometic scoring that caused NaN in output
-   Fixed bug that caused crashes on deletion of bookmarks
-   Fixed repeated widget update calls for peakgroup display from bookmarks 
-   Fixed / suppressed many compiler warning

## [8.0.2][]
### Added
-   Maven calcuates and displays purity of ms2 specta
-   Extract precursor isolation window from mzXML records

### Changed

### Deprecated

### Removed

### Fixed

## [8.0.1][]
### Added
-   Detect dual 13C/2H labeled isotopes @lparsons

### Changed

### Deprecated

### Removed

### Fixed
-   Fixed isotopes widget to display only those isotopes that correspond the
    isotopes selected in the application preferences (i.e. if C13 is not
    selected in preferences, then no C13 isotopes will be displayed). @lparsons

### Security


## [8.0.0][] - 2017-08-28
### Added
-   Build process automated using Travis and Appveyor @lparsons
-   Tagged release versions pushed to GitHub Releases @lparsons
-   TODO: Many other changes @eugenemel

### Changed

### Deprecated

### Removed

### Fixed

### Security


## [20161028][] - 2016-10-28
-   TODO - Document changes

[8.0.1]: https://github.com/eugenemel/maven/compare/8.0.0...8.0.1
[8.0.0]: https://github.com/eugenemel/maven/compare/20161028...8.0.0
[20161028]: https://github.com/eugenemel/maven/compare/92cf1d16dfc7d9f6bf4394890a06c3ecbed2ba1a...20161028
