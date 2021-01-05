# spectro
Terminal Based Spectrogram Viewer

Features:
* Analyze the frequencies present in WAV files
* Display intensity of range of frequency using colored ASCII
* Display the precise intensity of selected frequencies 

### Help
For information about usage, call

    $ ./spectro --help

The tool takes an audio file (currently only WAV is supported) and performs a discrete fourier analysis on a selected channel.
A range of frequencies are analyzed and the output is displayed in a table.
Specified frequencies may also be analyzed to show the exact intensity.

### Build
To make `spectro`, call

    $ make spectro

It requires the [ALSA](https://www.alsa-project.org/wiki/Main_Page "Advanced Linux Sound Library") library files to build.


### Sample Output
The below output is the uncolored spectrogram produced for an interval of a recording of bird chirps,

```
Sampling Frequency: 44100Hz		Duration: 85.0286s		Channels: 2
+---------+--------------------------------------------------+
|  Time   | 100.0                                    10000.0 |
+---------+--------------------------------------------------+
|  17.000 |                                                  |
|  17.500 |                                        `         |
|  18.000 |                                    `  ``         |
|  18.500 |                                    ' "`          |
|  19.000 |              ``   `'```            '*`   `       |
|  19.500 |                ` ``   `          ` ``'`# `       |
|  20.000 |                                `  *'#*##``       |
|  20.500 |                                ``%`#*##"         |
|  21.000 |                                ******'`#         |
|  21.500 |                                                  |
|  22.000 |                                        `         |
|  22.500 |                                       '`' ##     |
|  23.000 |                 `                  "#####'``     |
|  23.500 |                     `` `       ` `'######`       |
|  24.000 |                 `              '*'`# ####"       |
|  24.500 |                     `           `*`% "' "`       |
|  25.000 |                                   ``####'        |
|  25.500 |                                 ''##'##*`        |
|  26.000 |                                ` **"#'#`         |
|  26.500 |                                     '            |
|  27.000 |                                                  |
|  27.500 |                                                  |
+---------+--------------------------------------------------+
```

The characters ` `, `` ` ``, `'`, `"`, `*`, `%`, and `#` are used to represent increasing intensity, in that order.
