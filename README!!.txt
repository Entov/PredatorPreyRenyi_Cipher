#PredatorPrey Encryption and Decryption
This repository contains a lightweight predator-prey based multimedia encryption program that operates on **raw binary data**. Since the program reads files as binary streams, it can be used with **any file type**.
## Programs

- `EPredatorPreyTI` — encrypts a file
- `DPredatorPreyTI` — decrypts a file
- Source, object, and executable files are included (`.c`, `.o`, `.exe`)

## Test files

Some test data are included in the repository.  
The file `barbara_binaria.pgm` is intentionally included for test BUT users may provide their own test image or file c:

## How it works

The encryption program reads the entire input file in binary form and encrypts all of it. In general, this includes the file headers, so the encrypted file is not directly viewable or usable in its original format.

A special visual case was used for `.pgm` images during the experiments, where the encrypted image can still be displayed as noise for analysis purposes.

## Usage

Open a terminal (or Command Prompt), move to the directory where the program is located, and run:

### Encryption

```bash
EPredatorPreyTI.exe input_filename.extension

Example:
EPredatorPreyTI.exe barbara_binaria.pgm

### Decryption

If the decryption program, the encrypted .bin file and the .txt key are in the same directory, run:

DPredatorPreyTI.exe extension

Example: 

DPredatorPreyTI.exe pgm

Important: The encrypted .bin file and the .txt key must be in the same directory.

### Notes
- The program is designed to work with any file type because it processes files as binary data.
- Encryption and decryption are performed using the companion programs EPredatorPreyTI and DPredatorPreyTI.
- For most file types, the encrypted output cannot be opened directly because the original structure and headers are encrypted as well.
- PGM images were used as a visual test case to make encryption results easier to inspect.