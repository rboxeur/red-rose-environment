2 stdcall SysAllocString(wstr)
3 stdcall SysReAllocString(ptr wstr)
4 stdcall SysAllocStringLen(wstr long)
5 stdcall SysReAllocStringLen(ptr ptr long)
6 stdcall SysFreeString(wstr)
7 stdcall SysStringLen(wstr)
8 stdcall VariantInit(ptr)
9 stdcall VariantClear(ptr)
10 stdcall VariantCopy(ptr ptr)
11 stdcall VariantCopyInd(ptr ptr)
12 stdcall VariantChangeType(ptr ptr long long)
13 stdcall VariantTimeToDosDateTime(double ptr ptr)
14 stdcall DosDateTimeToVariantTime(long long ptr)
15 stdcall SafeArrayCreate(long long ptr)
16 stdcall SafeArrayDestroy(ptr)
17 stdcall SafeArrayGetDim(ptr)
18 stdcall SafeArrayGetElemsize(ptr)
19 stdcall SafeArrayGetUBound(ptr long long)
20 stdcall SafeArrayGetLBound(ptr long long)
21 stdcall SafeArrayLock(ptr)
22 stdcall SafeArrayUnlock(ptr)
23 stdcall SafeArrayAccessData(ptr ptr)
24 stdcall SafeArrayUnaccessData(ptr)
25 stdcall SafeArrayGetElement(ptr ptr ptr)
26 stdcall SafeArrayPutElement(ptr ptr ptr)
27 stdcall SafeArrayCopy(ptr ptr)
28 stdcall DispGetParam(ptr long long ptr ptr)
29 stdcall DispGetIDsOfNames(ptr ptr long ptr)
30 stdcall DispInvoke(ptr ptr long long ptr ptr ptr ptr)
31 stdcall CreateDispTypeInfo(ptr long ptr)
32 stdcall CreateStdDispatch(ptr ptr ptr ptr)
33 stdcall RegisterActiveObject(ptr ptr long ptr)
34 stdcall RevokeActiveObject(long ptr)
35 stdcall GetActiveObject(ptr ptr ptr)
36 stdcall SafeArrayAllocDescriptor(long ptr)
37 stdcall SafeArrayAllocData(ptr)
38 stdcall SafeArrayDestroyDescriptor(ptr)
39 stdcall SafeArrayDestroyData(ptr)
40 stdcall SafeArrayRedim(ptr ptr)
41 stdcall SafeArrayAllocDescriptorEx(long long ptr)
42 stdcall SafeArrayCreateEx(long long ptr ptr)
43 stdcall SafeArrayCreateVectorEx(long long long ptr)
44 stdcall SafeArraySetRecordInfo(ptr ptr)
45 stdcall SafeArrayGetRecordInfo(ptr ptr)
46 stdcall VarParseNumFromStr(wstr long long ptr ptr)
47 stdcall VarNumFromParseNum(ptr ptr long ptr)
48 stdcall VarI2FromUI1(long ptr)
49 stdcall VarI2FromI4(long ptr)
50 stdcall VarI2FromR4(float ptr)
51 stdcall VarI2FromR8(double ptr)
52 stdcall VarI2FromCy(int64 ptr)
53 stdcall VarI2FromDate(double ptr)
54 stdcall VarI2FromStr(wstr long long ptr)
55 stdcall VarI2FromDisp(ptr long ptr)
56 stdcall VarI2FromBool(long ptr)
57 stdcall SafeArraySetIID(ptr ptr)
58 stdcall VarI4FromUI1(long ptr)
59 stdcall VarI4FromI2(long ptr)
60 stdcall VarI4FromR4(float ptr)
61 stdcall VarI4FromR8(double ptr)
62 stdcall VarI4FromCy(int64 ptr)
63 stdcall VarI4FromDate(double ptr)
64 stdcall VarI4FromStr(wstr long long ptr)
65 stdcall VarI4FromDisp(ptr long ptr)
66 stdcall VarI4FromBool(long ptr)
67 stdcall SafeArrayGetIID(ptr ptr)
68 stdcall VarR4FromUI1(long ptr)
69 stdcall VarR4FromI2(long ptr)
70 stdcall VarR4FromI4(long ptr)
71 stdcall VarR4FromR8(double ptr)
72 stdcall VarR4FromCy(int64 ptr)
73 stdcall VarR4FromDate(double ptr)
74 stdcall VarR4FromStr(wstr long long ptr)
75 stdcall VarR4FromDisp(ptr long ptr)
76 stdcall VarR4FromBool(long ptr)
77 stdcall SafeArrayGetVartype(ptr ptr)
78 stdcall VarR8FromUI1(long ptr)
79 stdcall VarR8FromI2(long ptr)
80 stdcall VarR8FromI4(long ptr)
81 stdcall VarR8FromR4(float ptr)
82 stdcall VarR8FromCy(int64 ptr)
83 stdcall VarR8FromDate(double ptr)
84 stdcall VarR8FromStr(wstr long long ptr)
85 stdcall VarR8FromDisp(ptr long ptr)
86 stdcall VarR8FromBool(long ptr)
87 stdcall VarFormat(ptr ptr long long long ptr)
88 stdcall VarDateFromUI1(long ptr)
89 stdcall VarDateFromI2(long ptr)
90 stdcall VarDateFromI4(long ptr)
91 stdcall VarDateFromR4(float ptr)
92 stdcall VarDateFromR8(double ptr)
93 stdcall VarDateFromCy(int64 ptr)
94 stdcall VarDateFromStr(wstr long long ptr)
95 stdcall VarDateFromDisp(ptr long ptr)
96 stdcall VarDateFromBool(long ptr)
97 stdcall VarFormatDateTime(ptr long long ptr)
98 stdcall VarCyFromUI1(long ptr)
99 stdcall VarCyFromI2(long ptr)
100 stdcall VarCyFromI4(long ptr)
101 stdcall VarCyFromR4(float ptr)
102 stdcall VarCyFromR8(double ptr)
103 stdcall VarCyFromDate(double ptr)
104 stdcall VarCyFromStr(wstr long long ptr)
105 stdcall VarCyFromDisp(ptr long ptr)
106 stdcall VarCyFromBool(long ptr)
107 stdcall VarFormatNumber(ptr long long long long long ptr)
108 stdcall VarBstrFromUI1(long long long ptr)
109 stdcall VarBstrFromI2(long long long ptr)
110 stdcall VarBstrFromI4(long long long ptr)
111 stdcall VarBstrFromR4(float long long ptr)
112 stdcall VarBstrFromR8(double long long ptr)
113 stdcall VarBstrFromCy(int64 long long ptr)
114 stdcall VarBstrFromDate(double long long ptr)
115 stdcall VarBstrFromDisp(ptr long long ptr)
116 stdcall VarBstrFromBool(long long long ptr)
117 stdcall VarFormatPercent(ptr long long long long long ptr)
118 stdcall VarBoolFromUI1(long ptr)
119 stdcall VarBoolFromI2(long ptr)
120 stdcall VarBoolFromI4(long ptr)
121 stdcall VarBoolFromR4(float ptr)
122 stdcall VarBoolFromR8(double ptr)
123 stdcall VarBoolFromDate(double ptr)
124 stdcall VarBoolFromCy(int64 ptr)
125 stdcall VarBoolFromStr(wstr long long ptr)
126 stdcall VarBoolFromDisp(ptr long ptr)
127 stdcall VarFormatCurrency(ptr long long long long long ptr)
128 stdcall VarWeekdayName(long long long long ptr)
129 stdcall VarMonthName(long long long ptr)
130 stdcall VarUI1FromI2(long ptr)
131 stdcall VarUI1FromI4(long ptr)
132 stdcall VarUI1FromR4(float ptr)
133 stdcall VarUI1FromR8(double ptr)
134 stdcall VarUI1FromCy(int64 ptr)
135 stdcall VarUI1FromDate(double ptr)
136 stdcall VarUI1FromStr(wstr long long ptr)
137 stdcall VarUI1FromDisp(ptr long ptr)
138 stdcall VarUI1FromBool(long ptr)
139 stdcall VarFormatFromTokens (ptr ptr ptr long ptr long)
140 stdcall VarTokenizeFormatString (ptr ptr long long long long ptr)
141 stdcall VarAdd(ptr ptr ptr)
142 stdcall VarAnd(ptr ptr ptr)
143 stdcall VarDiv(ptr ptr ptr)
144 stub OACreateTypeLib2
146 stdcall DispCallFunc(ptr long long long long ptr ptr ptr)
147 stdcall VariantChangeTypeEx(ptr ptr long long long)
148 stdcall SafeArrayPtrOfIndex(ptr ptr ptr)
149 stdcall SysStringByteLen(ptr)
150 stdcall SysAllocStringByteLen(ptr long)
152 stdcall VarEqv(ptr ptr ptr)
153 stdcall VarIdiv(ptr ptr ptr)
154 stdcall VarImp(ptr ptr ptr)
155 stdcall VarMod(ptr ptr ptr)
156 stdcall VarMul(ptr ptr ptr)
157 stdcall VarOr(ptr ptr ptr)
158 stdcall VarPow(ptr ptr ptr)
159 stdcall VarSub(ptr ptr ptr)
160 stdcall CreateTypeLib(long wstr ptr)
161 stdcall LoadTypeLib (wstr ptr)
162 stdcall LoadRegTypeLib (ptr long long long ptr)
163 stdcall RegisterTypeLib(ptr wstr wstr)
164 stdcall QueryPathOfRegTypeLib(ptr long long long ptr)
165 stdcall LHashValOfNameSys(long long wstr)
166 stdcall LHashValOfNameSysA(long long str)
167 stdcall VarXor(ptr ptr ptr)
168 stdcall VarAbs(ptr ptr)
169 stdcall VarFix(ptr ptr)
170 stdcall OaBuildVersion()
171 stdcall ClearCustData(ptr)
172 stdcall VarInt(ptr ptr)
173 stdcall VarNeg(ptr ptr)
174 stdcall VarNot(ptr ptr)
175 stdcall VarRound(ptr long ptr)
176 stdcall VarCmp(ptr ptr long long)
177 stdcall VarDecAdd(ptr ptr ptr)
178 stdcall VarDecDiv(ptr ptr ptr)
179 stdcall VarDecMul(ptr ptr ptr)
180 stdcall CreateTypeLib2(long wstr ptr)
181 stdcall VarDecSub(ptr ptr ptr)
182 stdcall VarDecAbs(ptr ptr)
183 stdcall LoadTypeLibEx (wstr long ptr)
184 stdcall SystemTimeToVariantTime(ptr ptr)
185 stdcall VariantTimeToSystemTime(double ptr)
186 stdcall UnRegisterTypeLib (ptr long long long long)
187 stdcall VarDecFix(ptr ptr)
188 stdcall VarDecInt(ptr ptr)
189 stdcall VarDecNeg(ptr ptr)
190 stdcall VarDecFromUI1(long ptr)
191 stdcall VarDecFromI2(long ptr)
192 stdcall VarDecFromI4(long ptr)
193 stdcall VarDecFromR4(float ptr)
194 stdcall VarDecFromR8(double ptr)
195 stdcall VarDecFromDate(double ptr)
196 stdcall VarDecFromCy(int64 ptr)
197 stdcall VarDecFromStr(wstr long long ptr)
198 stdcall VarDecFromDisp(ptr long ptr)
199 stdcall VarDecFromBool(long ptr)
200 stdcall -import GetErrorInfo(long ptr)
201 stdcall -import SetErrorInfo(long ptr)
202 stdcall -import CreateErrorInfo(ptr)
203 stdcall VarDecRound(ptr long ptr)
204 stdcall VarDecCmp(ptr ptr)
205 stdcall VarI2FromI1(long ptr)
206 stdcall VarI2FromUI2(long ptr)
207 stdcall VarI2FromUI4(long ptr)
208 stdcall VarI2FromDec(ptr ptr)
209 stdcall VarI4FromI1(long ptr)
210 stdcall VarI4FromUI2(long ptr)
211 stdcall VarI4FromUI4(long ptr)
212 stdcall VarI4FromDec(ptr ptr)
213 stdcall VarR4FromI1(long ptr)
214 stdcall VarR4FromUI2(long ptr)
215 stdcall VarR4FromUI4(long ptr)
216 stdcall VarR4FromDec(ptr ptr)
217 stdcall VarR8FromI1(long ptr)
218 stdcall VarR8FromUI2(long ptr)
219 stdcall VarR8FromUI4(long ptr)
220 stdcall VarR8FromDec(ptr ptr)
221 stdcall VarDateFromI1(long ptr)
222 stdcall VarDateFromUI2(long ptr)
223 stdcall VarDateFromUI4(long ptr)
224 stdcall VarDateFromDec(ptr ptr)
225 stdcall VarCyFromI1(long ptr)
226 stdcall VarCyFromUI2(long ptr)
227 stdcall VarCyFromUI4(long ptr)
228 stdcall VarCyFromDec(ptr ptr)
229 stdcall VarBstrFromI1(long long long ptr)
230 stdcall VarBstrFromUI2(long long long ptr)
231 stdcall VarBstrFromUI4(long long long ptr)
232 stdcall VarBstrFromDec(ptr long long ptr)
233 stdcall VarBoolFromI1(long ptr)
234 stdcall VarBoolFromUI2(long ptr)
235 stdcall VarBoolFromUI4(long ptr)
236 stdcall VarBoolFromDec(ptr ptr)
237 stdcall VarUI1FromI1(long ptr)
238 stdcall VarUI1FromUI2(long ptr)
239 stdcall VarUI1FromUI4(long ptr)
240 stdcall VarUI1FromDec(ptr ptr)
241 stdcall VarDecFromI1(long ptr)
242 stdcall VarDecFromUI2(long ptr)
243 stdcall VarDecFromUI4(long ptr)
244 stdcall VarI1FromUI1(long ptr)
245 stdcall VarI1FromI2(long ptr)
246 stdcall VarI1FromI4(long ptr)
247 stdcall VarI1FromR4(float ptr)
248 stdcall VarI1FromR8(double ptr)
249 stdcall VarI1FromDate(double ptr)
250 stdcall VarI1FromCy(int64 ptr)
251 stdcall VarI1FromStr(wstr long long ptr)
252 stdcall VarI1FromDisp(ptr long ptr)
253 stdcall VarI1FromBool(long ptr)
254 stdcall VarI1FromUI2(long ptr)
255 stdcall VarI1FromUI4(long ptr)
256 stdcall VarI1FromDec(ptr ptr)
257 stdcall VarUI2FromUI1(long ptr)
258 stdcall VarUI2FromI2(long ptr)
259 stdcall VarUI2FromI4(long ptr)
260 stdcall VarUI2FromR4(float ptr)
261 stdcall VarUI2FromR8(double ptr)
262 stdcall VarUI2FromDate(double ptr)
263 stdcall VarUI2FromCy(int64 ptr)
264 stdcall VarUI2FromStr(wstr long long ptr)
265 stdcall VarUI2FromDisp(ptr long ptr)
266 stdcall VarUI2FromBool(long ptr)
267 stdcall VarUI2FromI1(long ptr)
268 stdcall VarUI2FromUI4(long ptr)
269 stdcall VarUI2FromDec(ptr ptr)
270 stdcall VarUI4FromUI1(long ptr)
271 stdcall VarUI4FromI2(long ptr)
272 stdcall VarUI4FromI4(long ptr)
273 stdcall VarUI4FromR4(float ptr)
274 stdcall VarUI4FromR8(double ptr)
275 stdcall VarUI4FromDate(double ptr)
276 stdcall VarUI4FromCy(int64 ptr)
277 stdcall VarUI4FromStr(wstr long long ptr)
278 stdcall VarUI4FromDisp(ptr long ptr)
279 stdcall VarUI4FromBool(long ptr)
280 stdcall VarUI4FromI1(long ptr)
281 stdcall VarUI4FromUI2(long ptr)
282 stdcall VarUI4FromDec(ptr ptr)
283 stdcall BSTR_UserSize(ptr long ptr)
284 stdcall BSTR_UserMarshal(ptr ptr ptr)
285 stdcall BSTR_UserUnmarshal(ptr ptr ptr)
286 stdcall BSTR_UserFree(ptr ptr)
287 stdcall VARIANT_UserSize(ptr long ptr)
288 stdcall VARIANT_UserMarshal(ptr ptr ptr)
289 stdcall VARIANT_UserUnmarshal(ptr ptr ptr)
290 stdcall VARIANT_UserFree(ptr ptr)
291 stdcall LPSAFEARRAY_UserSize(ptr long ptr)
292 stdcall LPSAFEARRAY_UserMarshal(ptr ptr ptr)
293 stdcall LPSAFEARRAY_UserUnmarshal(ptr ptr ptr)
294 stdcall LPSAFEARRAY_UserFree(ptr ptr)
295 stub LPSAFEARRAY_Size
296 stub LPSAFEARRAY_Marshal
297 stub LPSAFEARRAY_Unmarshal
298 stdcall VarDecCmpR8(ptr double)
299 stdcall VarCyAdd(int64 int64 ptr)
303 stdcall VarCyMul(int64 int64 ptr)
304 stdcall VarCyMulI4(int64 long ptr)
305 stdcall VarCySub(int64 int64 ptr)
306 stdcall VarCyAbs(int64 ptr)
307 stdcall VarCyFix(int64 ptr)
308 stdcall VarCyInt(int64 ptr)
309 stdcall VarCyNeg(int64 ptr)
310 stdcall VarCyRound(int64 long ptr)
311 stdcall VarCyCmp(int64 int64)
312 stdcall VarCyCmpR8(int64 double)
313 stdcall VarBstrCat(wstr wstr ptr)
314 stdcall VarBstrCmp(wstr wstr long long)
315 stdcall VarR8Pow(double double ptr)
316 stdcall VarR4CmpR8(float double)
317 stdcall VarR8Round(double long ptr)
318 stdcall VarCat(ptr ptr ptr)
319 stdcall VarDateFromUdateEx(ptr long long ptr)
322 stdcall GetRecordInfoFromGuids(ptr long long long ptr ptr)
323 stdcall GetRecordInfoFromTypeInfo(ptr ptr)
325 stub SetVarConversionLocaleSetting
326 stub GetVarConversionLocaleSetting
327 stdcall SetOaNoCache()
329 stdcall VarCyMulI8(int64 int64 ptr)
330 stdcall VarDateFromUdate(ptr long ptr)
331 stdcall VarUdateFromDate(double long ptr)
332 stdcall GetAltMonthNames(long ptr)
333 stdcall VarI8FromUI1(long long)
334 stdcall VarI8FromI2(long long)
335 stdcall VarI8FromR4(float long)
336 stdcall VarI8FromR8(double long)
337 stdcall VarI8FromCy(int64 ptr)
338 stdcall VarI8FromDate(double long)
339 stdcall VarI8FromStr(wstr long long ptr)
340 stdcall VarI8FromDisp(ptr long ptr)
341 stdcall VarI8FromBool(long long)
342 stdcall VarI8FromI1(long long)
343 stdcall VarI8FromUI2(long long)
344 stdcall VarI8FromUI4(long long)
345 stdcall VarI8FromDec(ptr ptr)
346 stdcall VarI2FromI8(int64 ptr)
347 stdcall VarI2FromUI8(int64 ptr)
348 stdcall VarI4FromI8(int64 ptr)
349 stdcall VarI4FromUI8(int64 ptr)
360 stdcall VarR4FromI8(int64 ptr)
361 stdcall VarR4FromUI8(int64 ptr)
362 stdcall VarR8FromI8(int64 ptr)
363 stdcall VarR8FromUI8(int64 ptr)
364 stdcall VarDateFromI8(int64 ptr)
365 stdcall VarDateFromUI8(int64 ptr)
366 stdcall VarCyFromI8(int64 ptr)
367 stdcall VarCyFromUI8(int64 ptr)
368 stdcall VarBstrFromI8(int64 long long ptr)
369 stdcall VarBstrFromUI8(int64 long long ptr)
370 stdcall VarBoolFromI8(int64 ptr)
371 stdcall VarBoolFromUI8(int64 ptr)
372 stdcall VarUI1FromI8(int64 ptr)
373 stdcall VarUI1FromUI8(int64 ptr)
374 stdcall VarDecFromI8(int64 ptr)
375 stdcall VarDecFromUI8(int64 ptr)
376 stdcall VarI1FromI8(int64 ptr)
377 stdcall VarI1FromUI8(int64 ptr)
378 stdcall VarUI2FromI8(int64 ptr)
379 stdcall VarUI2FromUI8(int64 ptr)
380 stub UserHWND_from_local
381 stub UserHWND_to_local
382 stub UserHWND_free_inst
383 stub UserHWND_free_local
384 stub UserBSTR_from_local
385 stub UserBSTR_to_local
386 stub UserBSTR_free_inst
387 stub UserBSTR_free_local
388 stub UserVARIANT_from_local
389 stub UserVARIANT_to_local
390 stub UserVARIANT_free_inst
391 stub UserVARIANT_free_local
392 stub UserEXCEPINFO_from_local
393 stub UserEXCEPINFO_to_local
394 stub UserEXCEPINFO_free_inst
395 stub UserEXCEPINFO_free_local
396 stub UserMSG_from_local
397 stub UserMSG_to_local
398 stub UserMSG_free_inst
399 stub UserMSG_free_local
401 stdcall OleLoadPictureEx(ptr long long ptr long long long ptr)
402 stdcall OleLoadPictureFileEx(int128 long long long ptr) OleLoadPictureFileEx
411 stdcall SafeArrayCreateVector(long long long)
412 stdcall SafeArrayCopyData(ptr ptr)
413 stdcall VectorFromBstr(ptr ptr)
414 stdcall BstrFromVector(ptr ptr)
415 stdcall OleIconToCursor(long long)
416 stdcall OleCreatePropertyFrameIndirect(ptr)
417 stdcall OleCreatePropertyFrame(ptr long long ptr long ptr long ptr ptr long ptr)
418 stdcall OleLoadPicture(ptr long long ptr ptr)
419 stdcall OleCreatePictureIndirect(ptr ptr long ptr)
420 stdcall OleCreateFontIndirect(ptr ptr ptr)
421 stdcall OleTranslateColor(long long ptr)
422 stdcall OleLoadPictureFile(int128 ptr)
423 stdcall OleSavePictureFile(ptr wstr)
424 stdcall OleLoadPicturePath(wstr ptr long long ptr ptr)
425 stdcall VarUI4FromI8(int64 ptr)
426 stdcall VarUI4FromUI8(int64 ptr)
427 stdcall VarI8FromUI8(int64 ptr)
428 stdcall VarUI8FromI8(int64 ptr)
429 stdcall VarUI8FromUI1(long ptr)
430 stdcall VarUI8FromI2(long ptr)
431 stdcall VarUI8FromR4(float ptr)
432 stdcall VarUI8FromR8(double ptr)
433 stdcall VarUI8FromCy(int64 ptr)
434 stdcall VarUI8FromDate(double ptr)
435 stdcall VarUI8FromStr(wstr long long ptr)
436 stdcall VarUI8FromDisp(ptr long ptr)
437 stdcall VarUI8FromBool(long ptr)
438 stdcall VarUI8FromI1(long ptr)
439 stdcall VarUI8FromUI2(long ptr)
440 stdcall VarUI8FromUI4(long ptr)
441 stdcall VarUI8FromDec(long ptr)
442 stdcall RegisterTypeLibForUser(ptr wstr wstr)
443 stdcall UnRegisterTypeLibForUser(ptr long long long long)

@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
