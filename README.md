## Predator-Prey Rényi Encryption and Decryption

This repository contains a lightweight predator-prey based multimedia encryption program that operates on **raw binary data**. Since the program reads files as binary streams, it can be used with **any file type**.

## Repository structure

```text
src/
├── EPredatorPreyTI.c
└── DPredatorPreyTI.c

data/
├── samples/
│   ├── images/
│   └── video/
└── metadata/

results/
├── correlation_histogram_15_rounds/
├── differential_attack_15_rounds/
├── entropy_15_rounds/
├── key_sensitivity_15_rounds/
├── nist_sp800_22/
└── speed/

docs/
└── original_notes.md
```

## Programs

The source code is located in `src/`:

- `EPredatorPreyTI.c` — encrypts a file
  
- `DPredatorPreyTI.c` — decrypts a file
  
Compiled files such as .o and .exe are not included in the repository. They should be generated locally.

## Test files

Some test data are included in the repository under:

data/samples/

For example, binary PGM images are available in:

data/samples/images/

Users may also provide their own test image, video, or binary file.

## How it works

The encryption program reads the entire input file in binary form and encrypts all of it. In general, this includes the file headers, so the encrypted file is not directly viewable or usable in its original format.

A special visual case was used for .pgm images during the experiments, where the encrypted image can still be displayed as noise for analysis purposes.

## Usage

First, compile the programs:

gcc src/EPredatorPreyTI.c -o EPredatorPreyTI

gcc src/DPredatorPreyTI.c -o DPredatorPreyTI

### Encryption

```bash
EPredatorPreyTI.exe input_filename.extension
```
Example:

```bash
EPredatorPreyTI.exe barbara_binaria.pgm
```

### Decryption

If the decryption program, the encrypted .bin file and the .txt key are in the same directory, run:

```bash
DPredatorPreyTI.exe extension
```

Example: 

```bash
DPredatorPreyTI.exe pgm
```

Important: The encrypted .bin file and the .txt key must be in the same directory.

### Experimental results

The repository includes selected experimental outputs in results/, including:

- Correlation and histogram analysis.
  
- Differential attack experiments.
  
- Entropy analysis.
  
- Key sensitivity experiments.
  
- NIST SP 800-22 statistical test reports.
  
- Speed measurements.

### Notes

- The program is designed to work with any file type because it processes files as binary data.
  
- Encryption and decryption are performed using the companion programs EPredatorPreyTI and DPredatorPreyTI.
  
- For most file types, the encrypted output cannot be opened directly because the original structure and headers are encrypted as well.
  
- PGM images were used as a visual test case to make encryption results easier to inspect.

## Visual overview

### Key sensitivity example

The following figure illustrates the effect of a minimal key modification. With the correct key, the image is recovered successfully; with a modified key (modifying the least significant bit of the first parameter of the key), the recovered output remains noise-like:

<img width="1786" height="495" alt="Key_sensitivity15LSB1" src="https://github.com/user-attachments/assets/d0b35b64-9ec4-4ac4-b4a4-2ab8e021d864" />


