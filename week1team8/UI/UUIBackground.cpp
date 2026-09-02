#include "UUIBackground.h"

void UUIBackground::Render(ID2D1RenderTarget* renderTarget)
{
	if (bgBitmap && renderTarget)
	{
		renderTarget->DrawBitmap(
			bgBitmap,
			&bgRect,
			1.0f,
			D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
		);
	}
}