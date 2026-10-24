/*
  The Forgotten Client
  Copyright (C) 2020 Saiyans King

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "GUI_ListBox.h"
#include "../engine.h"
#include "../thingManager.h"
#include "../GUI/itemUI.h"

extern Engine g_engine;
extern ThingManager g_thingManager;
extern Uint32 g_frameTime;

namespace
{
	enum ListBoxIconType : Uint8
	{
		LISTBOX_ICON_NONE = 0,
		LISTBOX_ICON_ITEM,
		LISTBOX_ICON_CREATURE
	};
}

GUI_ListBox::GUI_ListBox(iRect boxRect, Uint32 internalID) : m_scrollBar(iRect(0, 0, 0, 0), 0, 0)
{
	setRect(boxRect);
	m_internalID = internalID;
}

GUI_ListBox::~GUI_ListBox()
{
	m_listBox.clear();
	m_iconTypes.clear();
	m_iconIds.clear();
	m_iconColors.clear();
	m_itemIcons.clear();
}

void GUI_ListBox::setEventCallback(void (*eventHandlerFunction)(Uint32, Sint32), Uint32 mEvent)
{
	m_evtParam = mEvent;
	m_eventHandlerFunction = eventHandlerFunction;
}

void GUI_ListBox::setRect(iRect& NewRect)
{
	if(m_tRect == NewRect)
		return;

	iRect nRect = iRect(NewRect.x1 + NewRect.x2 - 13, NewRect.y1 + 1, 12, NewRect.y2 - 2);
	m_scrollBar.setRect(nRect);
	m_tRect = NewRect;
	// Rows start two pixels below the top and rendering stops sixteen pixels
	// before the bottom. Use the exact drawable row count; the former formula
	// overestimated some heights by one and made the final list entry impossible
	// to scroll into view.
	m_maxDisplay = UTIL_max<Sint32>(1, (m_tRect.y2 - 6) / m_rowHeight);
	m_scrollBar.setScrollSize(SDL_static_cast(Sint32, m_listBox.size()) - m_maxDisplay);
}

void GUI_ListBox::setSelect(Sint32 select)
{
	Sint32 lastSelect = m_select;
	m_select = UTIL_max<Sint32>(0, UTIL_min<Sint32>(SDL_static_cast(Sint32, m_listBox.size() - 1), select));
	m_scrollBar.setScrollPos(m_select - m_maxDisplay / 2);//Keep the scrollbar in the center
	if(lastSelect != m_select && m_eventHandlerFunction)
		UTIL_SafeEventHandler(m_eventHandlerFunction, m_evtParam, m_select);
}

void GUI_ListBox::add(const std::string data)
{
	m_listBox.push_back(std::move(data));
	m_iconTypes.push_back(LISTBOX_ICON_NONE);
	m_iconIds.push_back(0);
	m_iconColors.push_back(0);
	m_itemIcons.emplace_back();
	m_scrollBar.setScrollSize(SDL_static_cast(Sint32, m_listBox.size()) - m_maxDisplay);
}

void GUI_ListBox::addItem(const std::string data, Uint16 itemId)
{
	if(m_rowHeight != 32)
	{
		m_rowHeight = 32;
		m_maxDisplay = UTIL_max<Sint32>(1, (m_tRect.y2 - 6) / m_rowHeight);
	}
	m_listBox.push_back(std::move(data));
	m_iconTypes.push_back(LISTBOX_ICON_ITEM);
	m_iconIds.push_back(itemId);
	m_iconColors.push_back(0);
	m_itemIcons.emplace_back(ItemUI::createItemUI(itemId, 1));
	m_scrollBar.setScrollSize(SDL_static_cast(Sint32, m_listBox.size()) - m_maxDisplay);
}

void GUI_ListBox::addCreature(const std::string data, Uint16 lookType, Uint32 outfitColor)
{
	if(m_rowHeight != 32)
	{
		m_rowHeight = 32;
		m_maxDisplay = UTIL_max<Sint32>(1, (m_tRect.y2 - 6) / m_rowHeight);
	}
	m_listBox.push_back(std::move(data));
	m_iconTypes.push_back(LISTBOX_ICON_CREATURE);
	m_iconIds.push_back(lookType);
	m_iconColors.push_back(outfitColor);
	m_itemIcons.emplace_back();
	m_scrollBar.setScrollSize(SDL_static_cast(Sint32, m_listBox.size()) - m_maxDisplay);
}

void GUI_ListBox::set(Sint32 select, const std::string data)
{
	if(SDL_static_cast(size_t, select) < m_listBox.size())
		m_listBox[select] = std::move(data);
}

void GUI_ListBox::erase(Sint32 select)
{
	if(select == SDL_static_cast(Sint32, m_listBox.size() - 1))
		setSelect(m_select - 1);

	if(SDL_static_cast(size_t, select) < m_listBox.size())
	{
		m_listBox.erase(m_listBox.begin() + select);
		m_iconTypes.erase(m_iconTypes.begin() + select);
		m_iconIds.erase(m_iconIds.begin() + select);
		m_iconColors.erase(m_iconColors.begin() + select);
		m_itemIcons.erase(m_itemIcons.begin() + select);
		m_scrollBar.setScrollSize(SDL_static_cast(Sint32, m_listBox.size()) - m_maxDisplay);
	}
}

void GUI_ListBox::clear()
{
	m_listBox.clear();
	m_iconTypes.clear();
	m_iconIds.clear();
	m_iconColors.clear();
	m_itemIcons.clear();
	m_select = -1;
	m_scrollBar.setScrollPos(0);
	m_scrollBar.setScrollSize(0);
}

void GUI_ListBox::onKeyDown(SDL_Event& event)
{
	switch(event.key.keysym.sym)
	{
		case SDLK_UP:
		{
			m_select = UTIL_max<Sint32>(0, UTIL_min<Sint32>(SDL_static_cast(Sint32, m_listBox.size() - 1), m_select - 1));

			Sint32 actualPos = m_scrollBar.getScrollPos();
			Sint32 cachePos = m_select;
			if(actualPos > cachePos)
				m_scrollBar.setScrollPos(cachePos);
			if(m_eventHandlerFunction)
				UTIL_SafeEventHandler(m_eventHandlerFunction, m_evtParam, m_select);
			return;
		}

		case SDLK_DOWN:
		{
			m_select = UTIL_max<Sint32>(0, UTIL_min<Sint32>(SDL_static_cast(Sint32, m_listBox.size() - 1), m_select + 1));

			Sint32 actualPos = m_scrollBar.getScrollPos();
			Sint32 cachePos = m_select - m_maxDisplay + 1;
			if(actualPos < cachePos)
				m_scrollBar.setScrollPos(cachePos);
			if(m_eventHandlerFunction)
				UTIL_SafeEventHandler(m_eventHandlerFunction, m_evtParam, m_select);
			return;
		}

		default:
			break;
	}
}

void GUI_ListBox::onLMouseDown(Sint32 x, Sint32 y)
{
	iRect rect = iRect(m_tRect.x1 + 2, m_tRect.y1 + 2, m_tRect.x2 - 16, m_tRect.y2 - 4);
	if(rect.isPointInside(x, y))
	{
		Sint32 count = m_scrollBar.getScrollPos();
		std::vector<std::string>::iterator it = m_listBox.begin();
		std::advance(it, count);

		Sint32 posY = m_tRect.y1 + 2;
		Sint32 endY = m_tRect.y1 + m_tRect.y2 - 16;
		while(it != m_listBox.end())
		{
			if(posY > endY)
				break;

			rect = iRect(m_tRect.x1 + 2, posY, m_tRect.x2 - 16, m_rowHeight);
			if(rect.isPointInside(x, y))
			{
				if(g_frameTime < m_lastClick && m_select == count)
					m_doubleClicked = true;

				m_select = count;
				m_lastClick = g_frameTime + 1000;
				if(m_eventHandlerFunction)
					UTIL_SafeEventHandler(m_eventHandlerFunction, m_evtParam, m_select);
				return;
			}

			posY += m_rowHeight;
			++count;
			++it;
		}
		return;
	}
	if(m_scrollBar.getRect().isPointInside(x, y))
		m_scrollBar.onLMouseDown(x, y);
}

void GUI_ListBox::onLMouseUp(Sint32 x, Sint32 y)
{
	m_scrollBar.onLMouseUp(x, y);
	if(m_doubleClicked)
	{
		m_doubleClicked = false;
		if(m_eventHandlerFunction)
			UTIL_SafeEventHandler(m_eventHandlerFunction, m_evtParam, ~(m_select));
	}
}

void GUI_ListBox::onMouseMove(Sint32 x, Sint32 y, bool isInsideParent)
{
	m_scrollBar.onMouseMove(x, y, isInsideParent);
}

void GUI_ListBox::onWheel(Sint32, Sint32, bool wheelUP)
{
	if(wheelUP)
		m_scrollBar.setScrollPos(m_scrollBar.getScrollPos() - 1);
	else
		m_scrollBar.setScrollPos(m_scrollBar.getScrollPos() + 1);
}

void GUI_ListBox::render()
{
	auto& renderer = g_engine.getRender();
	renderer->drawPictureRepeat(GUI_UI_IMAGE, GUI_UI_ICON_HORIZONTAL_LINE_DARK_X, GUI_UI_ICON_HORIZONTAL_LINE_DARK_Y, GUI_UI_ICON_HORIZONTAL_LINE_DARK_W, GUI_UI_ICON_HORIZONTAL_LINE_DARK_H, m_tRect.x1, m_tRect.y1, m_tRect.x2, 1);
	renderer->drawPictureRepeat(GUI_UI_IMAGE, GUI_UI_ICON_VERTICAL_LINE_DARK_X, GUI_UI_ICON_VERTICAL_LINE_DARK_Y, GUI_UI_ICON_VERTICAL_LINE_DARK_W, GUI_UI_ICON_VERTICAL_LINE_DARK_H, m_tRect.x1, m_tRect.y1 + 1, 1, m_tRect.y2 - 1);
	renderer->drawPictureRepeat(GUI_UI_IMAGE, GUI_UI_ICON_HORIZONTAL_LINE_BRIGHT_X, GUI_UI_ICON_HORIZONTAL_LINE_BRIGHT_Y, GUI_UI_ICON_HORIZONTAL_LINE_BRIGHT_W, GUI_UI_ICON_HORIZONTAL_LINE_BRIGHT_H, m_tRect.x1 + 1, m_tRect.y1 + m_tRect.y2 - 1, m_tRect.x2 - 1, 1);
	renderer->drawPictureRepeat(GUI_UI_IMAGE, GUI_UI_ICON_VERTICAL_LINE_BRIGHT_X, GUI_UI_ICON_VERTICAL_LINE_BRIGHT_Y, GUI_UI_ICON_VERTICAL_LINE_BRIGHT_W, GUI_UI_ICON_VERTICAL_LINE_BRIGHT_H, m_tRect.x1 + m_tRect.x2 - 1, m_tRect.y1 + 1, 1, m_tRect.y2 - 2);
	renderer->fillRectangle(m_tRect.x1 + 1, m_tRect.y1 + 1, m_tRect.x2 - 14, m_tRect.y2 - 2, 64, 64, 64, 255);
	renderer->setClipRect(m_tRect.x1 + 2, m_tRect.y1 + 2, m_tRect.x2 - 16, m_tRect.y2 - 4);

	Sint32 count = m_scrollBar.getScrollPos();
	std::vector<std::string>::iterator it = m_listBox.begin();
	std::advance(it, count);

	Sint32 posY = m_tRect.y1 + 2;
	Sint32 endY = m_tRect.y1 + m_tRect.y2 - 16;
	while(it != m_listBox.end())
	{
		if(posY > endY)
			break;

		const size_t row = SDL_static_cast(size_t, count);
		if(count++ == m_select)
		{
			renderer->fillRectangle(m_tRect.x1 + 2, posY, m_tRect.x2 - 16, m_rowHeight, 112, 112, 112, 255);
		}

		Sint32 textX = m_tRect.x1 + 3;
		if(row < m_iconTypes.size() && m_iconTypes[row] != LISTBOX_ICON_NONE)
		{
			textX += 31;
			if(m_iconTypes[row] == LISTBOX_ICON_ITEM && row < m_itemIcons.size() && m_itemIcons[row])
				m_itemIcons[row]->render(m_tRect.x1 + 3, posY, 28);
			else if(m_iconTypes[row] == LISTBOX_ICON_CREATURE && m_iconIds[row] != 0)
			{
				ThingType* type = g_thingManager.getThingType(ThingCategory_Creature, m_iconIds[row]);
				if(type)
				{
					Uint32 sprite = type->getSprite(ThingFrameGroup_Idle, 0, 0, 0, DIRECTION_SOUTH, 0, 0, 0);
					if(sprite != 0)
					{
						if(type->m_frameGroup[ThingFrameGroup_Idle].m_layers > 1)
						{
							Uint32 mask = type->getSprite(ThingFrameGroup_Idle, 0, 0, 1, DIRECTION_SOUTH, 0, 0, 0);
							if(mask != 0)
								renderer->drawSpriteMask(sprite, mask, m_tRect.x1 + 3, posY, 28, 28, 0, 0, 32, 32, m_iconColors[row]);
							else
								renderer->drawSprite(sprite, m_tRect.x1 + 3, posY, 28, 28, 0, 0, 32, 32);
						}
						else
							renderer->drawSprite(sprite, m_tRect.x1 + 3, posY, 28, 28, 0, 0, 32, 32);
					}
				}
			}
		}

		const Uint8 color = (SDL_static_cast(Sint32, row) == m_select ? 255 : 175);
		g_engine.drawFont(CLIENT_FONT_NONOUTLINED, textX, posY + (m_rowHeight - 10) / 2, (*it), color, color, color, CLIENT_FONT_ALIGN_LEFT);

		posY += m_rowHeight;
		++it;
	}
	renderer->disableClipRect();
	m_scrollBar.render();
}
