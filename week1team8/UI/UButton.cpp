#include "UButton.h"

void UUIButton::Render(ID2D1RenderTarget* renderTarget)
{
	if (ButtonBitmap && renderTarget)
	{
		renderTarget->DrawBitmap(
			ButtonBitmap,
			&ButtonRect,
			1.0f,
			D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
		);
	}
}
