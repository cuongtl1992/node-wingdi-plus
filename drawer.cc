#include <nan.h>
#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <strsafe.h>
#include <fstream>
#include <string>
#include <locale.h>
#include <gdiplus.h>
#include <objidl.h>
#include <atlconv.h>
#include <Shlwapi.h>
#pragma comment(lib,"gdiplus.lib")
#pragma comment(lib,"Shlwapi.lib")
using namespace v8;
using namespace Gdiplus;

HDC createDCWithoutDialog(const char *printer) {
	int nChars = MultiByteToWideChar(CP_UTF8, 0, printer, -1, NULL, 0);
	wchar_t* printerName = (wchar_t*)malloc(nChars * sizeof(wchar_t));
	wmemset(printerName, 0x00, nChars);
	MultiByteToWideChar(CP_UTF8, 0, printer, -1, (LPWSTR)printerName, nChars);

	HDC hdcPrint;
	hdcPrint = CreateDCW(NULL, printerName, NULL, NULL);
	int lastErrorNumber = GetLastError();

	if (hdcPrint == NULL) {
		char message[500];
		strcat(message, "createDC failed with code ");
		strcat(message, std::to_string(lastErrorNumber).c_str());
		strcat(message, ", printer: ");
		strcat(message, printer);
		Nan::ThrowTypeError(message);
		return NULL;
	}
	return hdcPrint;
}

const char* ToCString(const String::Utf8Value& value) {
	return *value ? *value : "<string conversion failed>";
}

/*
	@author: CuongTL
	@description: Print image from file
	@args[0]: printer name
	@args[1]: print job name
	@args[2]: path to image file
	@return: print job id
*/
void printImageFromFile(const Nan::FunctionCallbackInfo<Value>& args) {
	if (args.Length() < 3) {
		Nan::ThrowTypeError("wrong number of arguments");
		return;
	}

	if (!args[0]->IsString()) {
		Nan::ThrowTypeError("Printer name must is string");
		return;
	}

	if (!args[1]->IsString()) {
		Nan::ThrowTypeError("Print job name must is string");
		return;
	}

	if (!args[2]->IsString()) {
		Nan::ThrowTypeError("Path to image file must is string");
		return;
	}

	// Parse arguments
	std::string val = *String::Utf8Value(args[0]);
	const char *printerName = val.c_str();
	String::Utf8Value str1(args[1]);
	LPCSTR printJobName = ToCString(str1);
	String::Utf8Value str2(args[2]);
	std::string imagePath = ToCString(str2);
	std::cout << "Printing image from file with arguments:" << std::endl;
	std::cout << "Printer: " + std::string(printerName) << std::endl;
	std::cout << "Print Job: " + std::string(printJobName) << std::endl;
	std::cout << "Image path: " + imagePath << std::endl;

	// Initialize GDI+
	ULONG_PTR gdiplusToken;
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// Define variables
	HDC hdcPrint = createDCWithoutDialog(printerName);
	UINT dimCount, frameCount;
	GUID* pDimIDs;
	GUID pageGuid;
	float width, height;
	DOCINFO docInfo = { sizeof(DOCINFO), printJobName };
	Graphics* graphics = Graphics::FromHDC(hdcPrint);

	int printJobId = StartDoc(hdcPrint, &docInfo);

	// Loading image from file
	USES_CONVERSION;
	Image* image = Image::FromFile(A2W(imagePath.c_str()));

	// Get the list of frame dimensions from the Image object
	dimCount = image->GetFrameDimensionsCount();
	pDimIDs = new GUID[dimCount];

	// Get the number of frames (pages) in the first dimension
	image->GetFrameDimensionsList(pDimIDs, dimCount);
	frameCount = image->GetFrameCount(&pDimIDs[0]);

	// Not use de-constructor
	delete pDimIDs;

	pageGuid = FrameDimensionPage;

	for (UINT i = 0; i < frameCount; i++)
	{
		StartPage(hdcPrint);
		image->SelectActiveFrame(&pageGuid, i);
		graphics->SetPageUnit(UnitInch);
		// width = (image_width * 25.4mm / 203 dpi) / 25.4
		width = (image->GetWidth() * 25.4 / (double)GetDeviceCaps(hdcPrint, LOGPIXELSX)) / 25.4;
		// height = (image_height * 25.4mm / 203 dpi) / 25.4
		height = (image->GetHeight() * 25.4 / (double)GetDeviceCaps(hdcPrint, LOGPIXELSY)) / 25.4;
		graphics->DrawImage(image, 0.f, 0.f, width, height);
		EndPage(hdcPrint);
	}

	EndDoc(hdcPrint);
	DeleteDC(hdcPrint);
	delete graphics;
	delete image;
	GdiplusShutdown(gdiplusToken);
	args.GetReturnValue().Set(printJobId);
}

/*
	@author: CuongTL
	@description: Print image from buffer array
	@args[0]: printer name
	@args[1]: print job name
	@args[2]: buffer array
	@return: print job id
*/
void printImageFromBytes(const Nan::FunctionCallbackInfo<Value>& args) {
	if (args.Length() < 4) {
		Nan::ThrowTypeError("wrong number of arguments");
		return;
	}

	if (!args[0]->IsString()) {
		Nan::ThrowTypeError("Printer name must is string");
		return;
	}

	if (!args[1]->IsString()) {
		Nan::ThrowTypeError("Print job name must is string");
		return;
	}

	// Parse arguments
	std::string val = *String::Utf8Value(args[0]);
	const char* printerName = val.c_str();
	String::Utf8Value str1(args[1]);
	LPCSTR printJobName = ToCString(str1);
	char* imageBuffer = node::Buffer::Data(args[2]->ToObject());
	unsigned int imageSize = args[3]->Uint32Value();
	
	std::cout << "Printing image from buffer array with arguments:" << std::endl;
	std::cout << "Printer: " + std::string(printerName) << std::endl;
	std::cout << "Print Job: " + std::string(printJobName) << std::endl;

	// Initialize GDI+
	ULONG_PTR gdiplusToken;
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// Define variables
	HDC hdcPrint = createDCWithoutDialog(printerName);
	UINT dimCount, frameCount;
	GUID* pDimIDs;
	GUID pageGuid;
	DOCINFO docInfo = { sizeof(DOCINFO), printJobName };
	Graphics* graphics = Graphics::FromHDC(hdcPrint);
	
	int printJobId = StartDoc(hdcPrint, &docInfo);

	// Loading image from bytes
	IStream *istream = SHCreateMemStream((BYTE*)imageBuffer, imageSize);
	Image* image = Image::FromStream(istream, true);

	// Get the list of frame dimensions from the Image object
	dimCount = image->GetFrameDimensionsCount();
	pDimIDs = new GUID[dimCount];

	// Get the number of frames (pages) in the first dimension
	image->GetFrameDimensionsList(pDimIDs, dimCount);
	frameCount = image->GetFrameCount(&pDimIDs[0]);

	// Not use de-constructor
	delete pDimIDs;
	graphics->SetPageUnit(UnitInch);
	pageGuid = FrameDimensionPage;

	double printerMaxHeight = (double)GetDeviceCaps(hdcPrint, VERTSIZE) / 25.4;
	double imgPrintWidth = (image->GetWidth() * 25.4 / (double)GetDeviceCaps(hdcPrint, LOGPIXELSX)) / 25.4;
	double imgPrintHeight = (image->GetHeight() * 25.4 / (double)GetDeviceCaps(hdcPrint, LOGPIXELSY)) / 25.4;
	double printedHeight = 0.f;
	double printHeight = 0.f;

	for (UINT i = 0; i < frameCount; i++)
	{
		while (printedHeight < imgPrintHeight)
		{
			StartPage(hdcPrint);
			image->SelectActiveFrame(&pageGuid, i);

			if (printerMaxHeight < imgPrintHeight) 
			{
				printHeight = printerMaxHeight;
			}
			else 
			{
				printHeight = imgPrintHeight;
			}

			graphics->DrawImage(image, 0.f, printedHeight, imgPrintWidth, printHeight);
			EndPage(hdcPrint);

			printedHeight += printHeight;
		}
	}

	EndDoc(hdcPrint);
	DeleteDC(hdcPrint);
	delete graphics;
	delete image;
	GdiplusShutdown(gdiplusToken);
	args.GetReturnValue().Set(printJobId);
}

void getLastError(const Nan::FunctionCallbackInfo<Value>& args) {
    int ret = GetLastError();
    args.GetReturnValue().Set(ret);
}

void Init(v8::Local<v8::Object> exports) {
	// Export functions
	exports->Set(Nan::New("printImageFromFile").ToLocalChecked(),
		Nan::New<v8::FunctionTemplate>(printImageFromFile)->GetFunction());

	exports->Set(Nan::New("printImageFromBytes").ToLocalChecked(),
		Nan::New<v8::FunctionTemplate>(printImageFromBytes)->GetFunction());

	exports->Set(Nan::New("getLastError").ToLocalChecked(),
		Nan::New<v8::FunctionTemplate>(getLastError)->GetFunction());
}

NODE_MODULE(drawer, Init)


