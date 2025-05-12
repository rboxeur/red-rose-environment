# Red Rose: Wine from GE-Proton 9 with chaotic stuffs

<p align="center" width="100%">
    <img src='red-rose.png' align='center'/>
</p>

## About

This repository provides my Wine build to play native Windows games on Linux.

### wine-red-rose-base

It is a customized fork of [wine-ge-custom](https://github.com/GloriousEggroll/wine-ge-custom). 
It is based on latest Wine taken from Proton 9 (wine-bleeding-edge) from 04/25/2025. Then

- changes taken from [Glorious Eggroll proton](https://github.com/GloriousEggroll/proton-ge-custom) version 9-22 were applied
- a [patch was applied to remove any Steam dependancy](https://raw.githubusercontent.com/Frogging-Family/wine-tkg-git/refs/heads/master/wine-tkg-git/wine-tkg-patches/hotfixes/valve/de-steamify-90-be.mypatch) on this Wine version. Patch was taken from [Wine-TkG](https://github.com/Frogging-Family/wine-tkg-git.git).
- [Josh' theme patch](https://raw.githubusercontent.com/Frogging-Family/wine-tkg-git/refs/heads/master/wine-tkg-git/wine-tkg-patches/misc/josh-flat-theme/josh-flat-theme.patch) was applied.
- My minor changes were applied.

I keep these Wine source as-it-is as a reference

### wine-red-rose-custom 

This is the working Wine folder. Chosen cumulative MR (Merge Request) or BF (Bug Fixed) are applied

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


