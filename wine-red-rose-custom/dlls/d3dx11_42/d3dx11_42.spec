@ stdcall D3DX11CheckVersion(long long)
@ stdcall -import D3DX11CompileFromFileA(str ptr ptr str str long long ptr ptr ptr ptr)
@ stdcall -import D3DX11CompileFromFileW(wstr ptr ptr str str long long ptr ptr ptr ptr)
@ stdcall -import D3DX11CompileFromMemory(ptr long str ptr ptr str str long long ptr ptr ptr ptr)
@ stub D3DX11CompileFromResourceA
@ stub D3DX11CompileFromResourceW
@ stub D3DX11ComputeNormalMap
@ stub D3DX11CreateAsyncCompilerProcessor
@ stdcall -import D3DX11CreateAsyncFileLoaderA(str ptr)
@ stdcall -import D3DX11CreateAsyncFileLoaderW(wstr ptr)
@ stdcall -import D3DX11CreateAsyncMemoryLoader(ptr long ptr)
@ stdcall -import D3DX11CreateAsyncResourceLoaderA(long str ptr)
@ stdcall -import D3DX11CreateAsyncResourceLoaderW(long wstr ptr)
@ stub D3DX11CreateAsyncShaderPreprocessProcessor
@ stdcall -import D3DX11CreateAsyncShaderResourceViewProcessor(ptr ptr ptr)
@ stdcall -import D3DX11CreateAsyncTextureInfoProcessor(ptr ptr)
@ stdcall -import D3DX11CreateAsyncTextureProcessor(ptr ptr ptr)
@ stdcall -import D3DX11CreateShaderResourceViewFromFileA(ptr str ptr ptr ptr ptr)
@ stdcall -import D3DX11CreateShaderResourceViewFromFileW(ptr wstr ptr ptr ptr ptr)
@ stdcall -import D3DX11CreateShaderResourceViewFromMemory(ptr ptr long ptr ptr ptr ptr)
@ stdcall -import D3DX11CreateShaderResourceViewFromResourceA(ptr long str ptr ptr ptr ptr)
@ stdcall -import D3DX11CreateShaderResourceViewFromResourceW(ptr long wstr ptr ptr ptr ptr)
@ stdcall -import D3DX11CreateTextureFromFileA(ptr str ptr ptr ptr ptr)
@ stdcall -import D3DX11CreateTextureFromFileW(ptr wstr ptr ptr ptr ptr)
@ stdcall -import D3DX11CreateTextureFromMemory(ptr ptr long ptr ptr ptr ptr)
@ stdcall -import D3DX11CreateTextureFromResourceA(ptr long str ptr ptr ptr ptr)
@ stdcall -import D3DX11CreateTextureFromResourceW(ptr long wstr ptr ptr ptr ptr)
@ stdcall -import D3DX11CreateThreadPump(long long ptr)
@ stdcall -import D3DX11FilterTexture(ptr ptr long long)
@ stdcall -import D3DX11GetImageInfoFromFileA(str ptr ptr ptr)
@ stdcall -import D3DX11GetImageInfoFromFileW(wstr ptr ptr ptr)
@ stdcall -import D3DX11GetImageInfoFromMemory(ptr long ptr ptr ptr)
@ stdcall -import D3DX11GetImageInfoFromResourceA(long str ptr ptr ptr)
@ stdcall -import D3DX11GetImageInfoFromResourceW(long wstr ptr ptr ptr)
@ stdcall -import D3DX11LoadTextureFromTexture(ptr ptr ptr ptr)
@ stub D3DX11PreprocessShaderFromFileA
@ stub D3DX11PreprocessShaderFromFileW
@ stub D3DX11PreprocessShaderFromMemory
@ stub D3DX11PreprocessShaderFromResourceA
@ stub D3DX11PreprocessShaderFromResourceW
@ stub D3DX11SHProjectCubeMap
@ stdcall -import D3DX11SaveTextureToFileA(ptr ptr long str)
@ stdcall -import D3DX11SaveTextureToFileW(ptr ptr long wstr)
@ stdcall -import D3DX11SaveTextureToMemory(ptr ptr long ptr long)
