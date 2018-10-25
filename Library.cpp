#include <nn/nn_Log.h>  

#include <nn/swkbd/swkbd_Api.h>
#include <nn/swkbd/swkbd_Result.h>

#include <nn/nn_BitTypes.h>

#include <nn/nn_Common.h>
#include <nn/nn_Macro.h>
#include <nn/mem/mem_StandardAllocator.h>

#include <string.h>
using namespace std;

namespace {
	const size_t ApplicationHeapSize = 128 * 1024 * 1024;
}

class ApplicationHeap
{
	NN_DISALLOW_COPY(ApplicationHeap);
	NN_DISALLOW_MOVE(ApplicationHeap);


private:
	void* m_pMemoryHeap;
	nn::mem::StandardAllocator m_ApplicationHeapHandle;


public:
	ApplicationHeap() NN_NOEXCEPT;

	void Initialize(size_t size) NN_NOEXCEPT;

	void Finalize() NN_NOEXCEPT;

	void* Allocate(size_t size, size_t alignment) NN_NOEXCEPT;

	void* Allocate(size_t size) NN_NOEXCEPT;

	void Deallocate(void* ptr) NN_NOEXCEPT;

};



//ApplicationHeap.cpp
ApplicationHeap::ApplicationHeap() NN_NOEXCEPT
	: m_pMemoryHeap(0)
{
}

void ApplicationHeap::Initialize(size_t size) NN_NOEXCEPT
{
	m_pMemoryHeap = new nn::Bit8[size];

	m_ApplicationHeapHandle.Initialize(m_pMemoryHeap, size);
}

void ApplicationHeap::Finalize() NN_NOEXCEPT
{
	m_ApplicationHeapHandle.Finalize();

	delete[] reinterpret_cast<nn::Bit8*>(m_pMemoryHeap);
}

void* ApplicationHeap::Allocate(size_t size, size_t alignment) NN_NOEXCEPT
{
	return m_ApplicationHeapHandle.Allocate(size, alignment);
}

void* ApplicationHeap::Allocate(size_t size) NN_NOEXCEPT
{
	return this->Allocate(size, sizeof(void*));
}

void ApplicationHeap::Deallocate(void* ptr) NN_NOEXCEPT
{
	if (ptr != 0)
	{
		m_ApplicationHeapHandle.Free(ptr);
	}
}



extern "C" {

	int SoftwareKeyboardOpen(char *input)

	{
		char16_t *keyboard_input;

		ApplicationHeap applicationHeap;
		applicationHeap.Initialize(ApplicationHeapSize);


		nn::swkbd::ShowKeyboardArg showKeyboardArg;
		nn::swkbd::MakePreset(&(showKeyboardArg.keyboardConfig), nn::swkbd::Preset_Default);

		// 漢字入力に対応
		showKeyboardArg.keyboardConfig.keyboardMode = nn::swkbd::KeyboardMode_Full;
		showKeyboardArg.keyboardConfig.isPredictionEnabled = true;


		// ガイド文字列の設定
		const char* guide_string = u8"please input word.";
		nn::swkbd::SetGuideTextUtf8(&showKeyboardArg.keyboardConfig,
			guide_string);


		// 共有メモリ用バッファの割り当て
		size_t in_heap_size = nn::swkbd::GetRequiredWorkBufferSize(false);
		void* swkbd_work_buffer = applicationHeap.Allocate(in_heap_size, nn::os::MemoryPageSize);

		showKeyboardArg.workBuf = swkbd_work_buffer;
		showKeyboardArg.workBufSize = in_heap_size;

		// 終了パラメータの設定
		size_t out_heap_size = nn::swkbd::GetRequiredStringBufferSize();
		nn::swkbd::String output_string;
		output_string.ptr = reinterpret_cast<char16_t*>(
			applicationHeap.Allocate(out_heap_size, nn::os::MemoryPageSize));
		output_string.bufSize = out_heap_size;



		nn::Result result = nn::swkbd::ShowKeyboard(&output_string, showKeyboardArg);
		if (nn::swkbd::ResultCanceled::Includes(result))
		{
//			NN_LOG(" -- cancel --\n");
		}
		else if (result.IsSuccess())
		{
//			NN_LOG(" -- ok --\n");
		}

		// 結果文字列を受け取る
//		NN_LOG("SwkbdSimple: return :\n");
		keyboard_input = reinterpret_cast<char16_t*>(output_string.ptr);

		int str_index = 0;
		int input_counter = 0;

		while (keyboard_input[str_index] != 0)
		{

//			NN_LOG("0x%04x,", keyboard_input[str_index]);

			//2byteの入力文字を引数のポインタに1byteごとに代入
			input[input_counter] = (char)((keyboard_input[str_index] & 0xFF00) >> 8);
			input[input_counter + 1] = (char)(keyboard_input[str_index] & 0x00FF);	

//			NN_LOG("   0x%02x,", input[input_counter]);
//			NN_LOG(" 0x%02x,", input[input_counter + 1]);
//			NN_LOG("\n");
			str_index++;
			input_counter += 2;
		}

//		NN_LOG("\n");



		// 共有メモリ用バッファの解放
		applicationHeap.Deallocate(output_string.ptr);
		applicationHeap.Deallocate(swkbd_work_buffer);

		return(str_index);

	}

}


