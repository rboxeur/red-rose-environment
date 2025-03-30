/*
 * Copyright (C) 2024 Biswapriyo Nath
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __windows_graphics_directx_direct3d11_interop_h__
#define __windows_graphics_directx_direct3d11_interop_h__

#include <inspectable.h>
#include <dxgi.h>

EXTERN_C HRESULT WINAPI CreateDirect3D11DeviceFromDXGIDevice(IDXGIDevice *dxgi_device, IInspectable **graphics_device);
EXTERN_C HRESULT WINAPI CreateDirect3D11SurfaceFromDXGISurface(IDXGISurface* dgxi_surface, IInspectable **graphics_surface);

#if defined(__cplusplus)

namespace Windows {
    namespace Graphics {
        namespace DirectX {
            namespace Direct3D11 {
                MIDL_INTERFACE("a9b3d012-3df2-4ee3-b8d1-8695f457d3c1")
                IDirect3DDxgiInterfaceAccess : public IUnknown
                {
                    virtual HRESULT STDMETHODCALLTYPE GetInterface(REFIID iid, void **p) = 0;
                };
            }
        }
    }
}

#endif /* __cplusplus */
#endif /* __windows_graphics_directx_direct3d11_interop_h__ */
