# CRYENGINE
This repository houses the source code for CRYENGINE.

Instructions on getting started with git can be found [here](http://docs.cryengine.com/display/CEPROG/Getting+Started+with+git), along with details on working with launcher projects and git source code.


## Building
<<<<<<< HEAD
In order to compile, you will need to download the SDKs for the particular release you are trying to build. They can be found [here](https://github.com/CRYTEK-CRYENGINE/CRYENGINE/releases).

Extract the archive and move the SDK directory to the **Code** folder and rename it to **SDKs**. 
=======
In order to compile, you will need to download some thirdparty SDKs. They can be downloaded by running the *download_sdks.py* script.
Or on windows the *download_sdks.exe* can be used alternatively.
>>>>>>> upstream/stabilisation

To compile the engine the provided WAF has to be used. See [here](http://docs.cryengine.com/display/CEPROG/Getting+Started+with+WAF) for more information.


## Branches
Development takes place primarily in the "main" branch. The stabilisation branch is used for fixing bugs in the run-up to release, and the release branch provides stable snapshots of the engine.

To prepare for a major (feature) release, we integrate "main" into "stabilisation", and then continue fixing bugs in "stabilisation". To prepare for a minor (stability) release, individual changes from 'main are integrated directly into "stabilisation". In each case, when the release is deemed ready, "stabilisation" is integrated to "release".


## License
The source code in this repository is governed by the CRYENGINE license agreement, which is contained in LICENSE.md, adjacent to this file.
