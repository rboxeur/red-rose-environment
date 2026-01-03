# Red Rose: Wine from GE-Proton 8/9/10 with chaotic stuffs

<p align="center" width="100%">
    <img src='red-rose.png' align='center'/>
</p>

## About

This repository provides my Wine build to play native Windows games on Linux.

### wine-red-rose-base-08

It is a customized fork of [wine-ge-custom](https://github.com/GloriousEggroll/wine-ge-custom).
It is based on latest Wine tag taken from Proton 8 (wine-bleeding-edge) from 2024 [experimental-wine-bleeding-edge-8.0-82496-20240226-p479bb6-wa058f1-d9004c1-v806fd7].
Then

- changes taken from [Glorious Eggroll proton](https://github.com/GloriousEggroll/proton-ge-custom) version 8-26 were applied
- a [patch was applied to remove any Steam dependancy](https://raw.githubusercontent.com/Frogging-Family/wine-tkg-git/refs/heads/master/wine-tkg-git/wine-tkg-patches/hotfixes/valve/de-steamify-80-be.mypatch) on this Wine version. Patch was taken from [Wine-TkG](https://github.com/Frogging-Family/wine-tkg-git.git).
- [Josh' theme patch](https://raw.githubusercontent.com/Frogging-Family/wine-tkg-git/refs/heads/master/wine-tkg-git/wine-tkg-patches/misc/josh-flat-theme/josh-flat-theme.patch) was applied.
- My minor changes for Red Rose were applied.

I keep these Wine source as-it-is as a reference.


### wine-red-rose-base-09

It is a customized fork of [wine-ge-custom](https://github.com/GloriousEggroll/wine-ge-custom). 
It is based on latest Wine tag taken from Proton 9 (wine-bleeding-edge) from 2025 [experimental-wine-bleeding-edge-9.0-99996-20240527-p01a60c-w7374ef-d7df801-v32ca00]. 
Then

- changes taken from [Glorious Eggroll proton](https://github.com/GloriousEggroll/proton-ge-custom) version 9-22 were applied
- a [patch was applied to remove any Steam dependancy](https://raw.githubusercontent.com/Frogging-Family/wine-tkg-git/refs/heads/master/wine-tkg-git/wine-tkg-patches/hotfixes/valve/de-steamify-90-be.mypatch) on this Wine version. Patch was taken from [Wine-TkG](https://github.com/Frogging-Family/wine-tkg-git.git).
- [Josh' theme patch](https://raw.githubusercontent.com/Frogging-Family/wine-tkg-git/refs/heads/master/wine-tkg-git/wine-tkg-patches/misc/josh-flat-theme/josh-flat-theme.patch) was applied.
- My minor changes for Red Rose were applied.

I keep these Wine source as-it-is as a reference.

### wine-red-rose-base-10

Aligned with GE-Proton-10-28 [Glorious Eggroll proton](https://github.com/GloriousEggroll/proton-ge-custom) with following specifications

- [Patch to remove any Steam dependancy](https://raw.githubusercontent.com/Frogging-Family/wine-tkg-git/refs/heads/master/wine-tkg-git/wine-tkg-patches/hotfixes/valve/de-steamify-10.0-be.mypatch) was applied -- slightly modified.
- [Glorious Eggroll's patch for wayland](https://github.com/GloriousEggroll/proton-ge-custom/blob/master/patches/wine-hotfixes/wine-wayland/0003-use-surfaceless-for-GST.patch) was neutered.
- [Another Glorious Eggroll's patch for wayland](https://github.com/GloriousEggroll/proton-ge-custom/blob/master/patches/wine-hotfixes/wine-wayland/0305-winegstreamer-Fix-x11-gst-display.patch) was neutered.
- [Kron4ek' patch to neuter OpenCL](https://github.com/Kron4ek/Wine-Builds/blob/master/proton-opencl.patch) was applied
- [Josh' theme patch](https://raw.githubusercontent.com/Frogging-Family/wine-tkg-git/refs/heads/master/wine-tkg-git/wine-tkg-patches/misc/josh-flat-theme/josh-flat-theme.patch) was applied
- [Glorious Eggroll's patch for winealsa-override-channel-count](https://github.com/GloriousEggroll/proton-ge-custom/blob/master/patches/proton/winealsa-override-channel-count.patch) was slightly modified
- Applied personal changes for Red Rose.

I keep these Wine source as-it-is as a reference.

### wine-red-rose-custom 

This is the working Wine folder. Chosen cumulative MR (Merge Request) or BF (Bug Fixed) are applied keeping wine-red-rose-base-09 as the starting point.

- Wine is built using a customized/enhanced environment based on the one provided by default by [Kron4ek](https://github.com/Kron4ek/Wine-Builds)
- This Wine repository is purely experimental. I am doing all of this just for fun. I am not responsible if if could break your Linux system

Used repositories. Backporting Wine merge requests and bug fixed from Wine 9 and 10

To keep it up-to-date then a few patches were applied from repositories listed below. (chosen patches are re-written -- if needed)

- https://gitlab.winehq.org/wine/wine
- https://gitlab.winehq.org/wine/wine/-/merge_requests
- https://github.com/ValveSoftware/wine/tree/bleeding-edge
- https://github.com/GloriousEggroll/proton-ge-custom
- https://github.com/Kron4ek/Wine-Builds

### Notes regarding Proton 9 and Proton 10

- for wine-bleeding-edge Valve switched from Proton 9 to Proton 10 on Wednesday April 30, 2025.
- wine-bleeding-edge went into Proton 10 beta on Wednesday April 30, 2025.


