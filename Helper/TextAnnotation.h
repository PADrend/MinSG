/*
	This file is part of the MinSG library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_HELPER_TEXTANNOTATION_H
#define MINSG_HELPER_TEXTANNOTATION_H

#include <string>

namespace Geometry {
template<typename T_> class _Vec2;
typedef _Vec2<int> Vec2i;
template<typename T_> class _Vec3;
typedef _Vec3<float> Vec3f;
}
namespace Rendering {
class TextRenderer;
}
namespace Util {
class Color4f;
}
namespace MinSG {
class FrameContext;

/**
 * @brief Display of text annotation
 *
 * Functions that can be used to annotate the scene visually with text.
 *
 * @author Benjamin Eikel
 * @date 2013-07-12
 * @ingroup helper
 */
namespace TextAnnotation {

/**
 * Render text to annotate a 3D position. The 3D position is projected to the
 * screen. The projected position marks the beginning of the pin. The pin
 * direction and length is given as parameter. At the end of the pin, the text
 * is shown with a rectangle as background.
 *
 * @param frameContext Frame context that is used for projection and rendering
 * @param worldPos 3D position in world coordinates that marks the point for
 * annotation
 * @param pinVector 2D direction in screen coordinates that defines the length
 * and direction of the pin. If you do not want to have a pin, pass
 * @code(0, 0)@endcode.
 * @param pinWidth The line width that is used to draw the pin
 * @param backgroundColor Color of the background rectangle behind the text and
 * of the pin
 * @param textRenderer Renderer that is used to draw the text. The size and
 * appearance of the text can be changed by using a different renderer.
 * @param text The text string that is to be drawn
 * @param textColor Color of the text
 */
MINSGAPI void displayText(FrameContext & frameContext,
				 const Geometry::Vec3f & worldPos,
				 const Geometry::Vec2i & pinVector,
				 const float pinWidth,
				 const Util::Color4f & backgroundColor,
				 const Rendering::TextRenderer & textRenderer,
				 const std::string & text,
				 const Util::Color4f & textColor);

}

}

#endif /* MINSG_HELPER_TEXTANNOTATION_H */
