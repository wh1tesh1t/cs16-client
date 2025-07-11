/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// Message.cpp
//
// implementation of CHudMessage class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "vgui_parser.h"
#include "draw_util.h"


int CHudMessage::Init(void)
{
	HOOK_MESSAGE( gHUD.m_Message, HudText );
	HOOK_MESSAGE( gHUD.m_Message, GameTitle );
	HOOK_MESSAGE( gHUD.m_Message, HudTextPro );
	HOOK_MESSAGE( gHUD.m_Message, HudTextArgs );

	gHUD.AddHudElem(this);
	Reset();

	return 1;
}

int CHudMessage::VidInit( void )
{
	m_HUD_title_half = gHUD.GetSpriteIndex( "title_half" );
	m_HUD_title_life = gHUD.GetSpriteIndex( "title_life" );

	return 1;
}


void CHudMessage::Reset( void )
{
 	memset( m_pMessages, 0, sizeof( m_pMessages[0] ) * maxHUDMessages );
	memset( m_startTime, 0, sizeof( m_startTime[0] ) * maxHUDMessages );
	
	m_gameTitleTime = 0;
	m_pGameTitle = NULL;
}


float CHudMessage::FadeBlend( float fadein, float fadeout, float hold, float localTime )
{
	float fadeTime = fadein + hold;
	float fadeBlend;

	if ( localTime < 0 )
		return 0;

	if ( localTime < fadein )
	{
		fadeBlend = 1 - ((fadein - localTime) / fadein);
	}
	else if ( localTime > fadeTime )
	{
		if ( fadeout > 0 )
			fadeBlend = 1 - ((localTime - fadeTime) / fadeout);
		else
			fadeBlend = 0;
	}
	else
		fadeBlend = 1;

	return fadeBlend;
}


int	CHudMessage::XPosition( float x, int width, int totalWidth )
{
	int xPos;

	if ( x == -1 )
	{
		xPos = (ScreenWidth - width) / 2;
	}
	else
	{
		if ( x < 0 )
			xPos = (1.0 + x) * ScreenWidth - totalWidth;	// Alight right
		else
			xPos = x * ScreenWidth;
	}

	if ( xPos + width > ScreenWidth )
		xPos = ScreenWidth - width;
	else if ( xPos < 0 )
		xPos = 0;

	return xPos;
}


int CHudMessage::YPosition( float y, int height )
{
	int yPos;

	if ( y == -1 )	// Centered?
		yPos = (ScreenHeight - height) * 0.5;
	else
	{
		// Alight bottom?
		if ( y < 0 )
			yPos = (1.0 + y) * ScreenHeight - height;	// Alight bottom
		else // align top
			yPos = y * ScreenHeight;
	}

	if ( yPos + height > ScreenHeight )
		yPos = ScreenHeight - height;
	else if ( yPos < 0 )
		yPos = 0;

	return yPos;
}


void CHudMessage::MessageScanNextChar( void )
{
	int srcRed = m_parms.pMessage->r1;
	int srcGreen = m_parms.pMessage->g1;
	int srcBlue = m_parms.pMessage->b1;
	int destRed, destGreen, destBlue, blend;
	destRed = destGreen = destBlue = blend = 0;

	switch( m_parms.pMessage->effect )
	{
	// Fade-in / Fade-out
	case 0:
	case 1:
		destRed = destGreen = destBlue = 0;
		blend = m_parms.fadeBlend;
		break;

	case 2:
		m_parms.charTime += m_parms.pMessage->fadein;
		if ( m_parms.charTime > m_parms.time )
		{
			srcRed = srcGreen = srcBlue = 0;
			blend = 0;	// pure source
		}
		else
		{
			float deltaTime = m_parms.time - m_parms.charTime;

			destRed = destGreen = destBlue = 0;
			if ( m_parms.time > m_parms.fadeTime )
			{
				blend = m_parms.fadeBlend;
			}
			else if ( deltaTime > m_parms.pMessage->fxtime )
				blend = 0;	// pure dest
			else
			{
				destRed = m_parms.pMessage->r2;
				destGreen = m_parms.pMessage->g2;
				destBlue = m_parms.pMessage->b2;
				blend = 255 - (deltaTime * (1.0/m_parms.pMessage->fxtime) * 255.0 + 0.5);
			}
		}
		break;
	}
	if ( blend > 255 )
		blend = 255;
	else if ( blend < 0 )
		blend = 0;

	m_parms.r = ((srcRed * (255-blend)) + (destRed * blend)) >> 8;
	m_parms.g = ((srcGreen * (255-blend)) + (destGreen * blend)) >> 8;
	m_parms.b = ((srcBlue * (255-blend)) + (destBlue * blend)) >> 8;

	if ( m_parms.pMessage->effect == 1 && m_parms.charTime != 0 )
	{
		if ( m_parms.x >= 0 && m_parms.y >= 0 && (m_parms.x + gHUD.GetCharWidth( m_parms.text )) <= ScreenWidth )
			DrawUtils::TextMessageDrawChar( m_parms.x, m_parms.y, m_parms.text, m_parms.pMessage->r2, m_parms.pMessage->g2, m_parms.pMessage->b2 );
	}
}


void CHudMessage::MessageScanStart( void )
{
	switch( m_parms.pMessage->effect )
	{
	// Fade-in / out with flicker
	case 1:
	case 0:
		m_parms.fadeTime = m_parms.pMessage->fadein + m_parms.pMessage->holdtime;
		

		if ( m_parms.time < m_parms.pMessage->fadein )
		{
			m_parms.fadeBlend = ((m_parms.pMessage->fadein - m_parms.time) * (1.0/m_parms.pMessage->fadein) * 255);
		}
		else if ( m_parms.time > m_parms.fadeTime )
		{
			if ( m_parms.pMessage->fadeout > 0 )
				m_parms.fadeBlend = (((m_parms.time - m_parms.fadeTime) / m_parms.pMessage->fadeout) * 255);
			else
				m_parms.fadeBlend = 255; // Pure dest (off)
		}
		else
			m_parms.fadeBlend = 0;	// Pure source (on)
		m_parms.charTime = 0;

		if ( m_parms.pMessage->effect == 1 && (rand()%100) < 10 )
			m_parms.charTime = 1;
		break;

	case 2:
		m_parms.fadeTime = (m_parms.pMessage->fadein * m_parms.length) + m_parms.pMessage->holdtime;
		
		if ( m_parms.time > m_parms.fadeTime && m_parms.pMessage->fadeout > 0 )
			m_parms.fadeBlend = (((m_parms.time - m_parms.fadeTime) / m_parms.pMessage->fadeout) * 255);
		else
			m_parms.fadeBlend = 0;
		break;
	}
}


void CHudMessage::MessageDrawScan( client_textmessage_t *pMessage, float time )
{
	int i, j, length, width;
	const char *pText;
	unsigned char line[80];

	pText = pMessage->pMessage;
	// Count lines
	m_parms.lines = 1;
	m_parms.time = time;
	m_parms.pMessage = pMessage;
	length = 0;
	width = 0;
	m_parms.totalWidth = 0;
	Con_UtfProcessChar( 0 );
	while ( *pText )
	{
		if ( *pText == '\n' )
		{
			m_parms.lines++;
			if ( width > m_parms.totalWidth )
				m_parms.totalWidth = width;
			width = 0;
		}
		else
		{
			int uch = Con_UtfProcessChar( (unsigned char)*pText );

			if ( !uch )
			{
				pText++;
				continue;
			}
			width += gHUD.GetCharWidth( uch );
		}
		pText++;
		length++;
	}
	m_parms.length = length;
	m_parms.totalHeight = (m_parms.lines * gHUD.GetCharHeight());


	m_parms.y = YPosition( pMessage->y, m_parms.totalHeight );
	pText = pMessage->pMessage;

	m_parms.charTime = 0;

	MessageScanStart();

	for ( i = 0; i < m_parms.lines; i++ )
	{
		m_parms.lineLength = 0;
		m_parms.width = 0;
		while ( *pText && *pText != '\n' && m_parms.lineLength < sizeof (line) - 1)
		{
			unsigned char c = *pText;
			line[m_parms.lineLength] = c;
			m_parms.lineLength++;
			int uch = Con_UtfProcessChar( c );

			if ( !uch )
			{
				pText++;
				continue;
			}
			m_parms.width += gHUD.GetCharWidth( uch );
			pText++;
		}
		pText++;		// Skip LF
		line[m_parms.lineLength] = 0;

		m_parms.x = XPosition( pMessage->x, m_parms.width, m_parms.totalWidth );

		for ( j = 0; j < m_parms.lineLength; j++ )
		{
			m_parms.text = line[j];
			int next = m_parms.x + gHUD.GetCharWidth( m_parms.text );
			MessageScanNextChar();
			
			if ( m_parms.x >= 0 && m_parms.y >= 0 && next <= ScreenWidth )
				m_parms.x += DrawUtils::TextMessageDrawChar( m_parms.x, m_parms.y, m_parms.text, m_parms.r, m_parms.g, m_parms.b );
		}

		m_parms.y += gHUD.GetCharHeight();
	}
}


int CHudMessage::Draw( float fTime )
{
	int i, drawn;
	client_textmessage_t *pMessage;
	float endTime;

	drawn = 0;

	if ( m_gameTitleTime > 0 )
	{
		float localTime = gHUD.m_flTime - m_gameTitleTime;
		float brightness;

		// Maybe timer isn't set yet
		if ( m_gameTitleTime > gHUD.m_flTime )
			m_gameTitleTime = gHUD.m_flTime;

		if ( localTime > (m_pGameTitle->fadein + m_pGameTitle->holdtime + m_pGameTitle->fadeout) )
			m_gameTitleTime = 0;
		else
		{
			brightness = FadeBlend( m_pGameTitle->fadein, m_pGameTitle->fadeout, m_pGameTitle->holdtime, localTime );

			int halfWidth = gHUD.GetSpriteRect(m_HUD_title_half).Width();
			int fullWidth = halfWidth + gHUD.GetSpriteRect(m_HUD_title_life).Width();
			int fullHeight = gHUD.GetSpriteRect(m_HUD_title_half).Height();

			int x = XPosition( m_pGameTitle->x, fullWidth, fullWidth );
			int y = YPosition( m_pGameTitle->y, fullHeight );


			SPR_Set( gHUD.GetSprite(m_HUD_title_half), brightness * m_pGameTitle->r1, brightness * m_pGameTitle->g1, brightness * m_pGameTitle->b1 );
			SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect(m_HUD_title_half) );

			SPR_Set( gHUD.GetSprite(m_HUD_title_life), brightness * m_pGameTitle->r1, brightness * m_pGameTitle->g1, brightness * m_pGameTitle->b1 );
			SPR_DrawAdditive( 0, x + halfWidth, y, &gHUD.GetSpriteRect(m_HUD_title_life) );

			drawn = 1;
		}
	}
	// Fixup level transitions
	for ( i = 0; i < maxHUDMessages; i++ )
	{
		// Assume m_parms.time contains last time
		if ( m_pMessages[i] )
		{
			pMessage = m_pMessages[i];
			if ( m_startTime[i] > gHUD.m_flTime )
				m_startTime[i] = gHUD.m_flTime + m_parms.time - m_startTime[i] + 0.2;	// Server takes 0.2 seconds to spawn, adjust for this
		}
	}

	for ( i = 0; i < maxHUDMessages; i++ )
	{
		if ( m_pMessages[i] )
		{
			pMessage = m_pMessages[i];

			// This is when the message is over
			switch( pMessage->effect )
			{
			// TODO: HACK to prevent crashing
			default:
			case 0:
			case 1:
				endTime = m_startTime[i] + pMessage->fadein + pMessage->fadeout + pMessage->holdtime;
				break;
			
			// Fade in is per character in scanning messages
			case 2:
				endTime = m_startTime[i] + (pMessage->fadein * strlen( pMessage->pMessage )) + pMessage->fadeout + pMessage->holdtime;
				break;
			}

			if ( fTime <= endTime )
			{
				float messageTime = fTime - m_startTime[i];

				// Draw the message
				// effect 0 is fade in/fade out
				// effect 1 is flickery credits
				// effect 2 is write out (training room)
				MessageDrawScan( pMessage, messageTime );

				drawn++;
			}
			else
			{
				// The message is over
				if( !strcmp( m_pMessages[i]->pName, "Custom" ) )
				{
					delete[] m_pMessages[i]->pMessage;
				}
				m_pMessages[i] = NULL;
			}
		}
	}

	// Remember the time -- to fix up level transitions
	m_parms.time = gHUD.m_flTime;
	// Don't call until we get another message
	if ( !drawn )
		m_iFlags &= ~HUD_DRAW;

	return 1;
}


void CHudMessage::MessageAdd( const char *pName, float time )
{
	int i,j;
	client_textmessage_t *tempMessage;

	for ( i = 0; i < maxHUDMessages; i++ )
	{
		if ( !m_pMessages[i] )
		{
			// Trim off a leading # if it's there
			if ( pName[0] == '#' ) 
				tempMessage = TextMessageGet( pName+1 );
			else
				tempMessage = TextMessageGet( pName );

			client_textmessage_t *message;
			if( tempMessage )
			{
				if( tempMessage->pMessage[0] == '#' )
				{
					message = AllocMessage( CHudTextMessage::BufferedLocaliseTextString( tempMessage->pMessage ), tempMessage );
				}
				else if( !strcmp( tempMessage->pName, "Custom" ) ) // Hey, it's mine way of detecting allocated message
				{
					message = AllocMessage( tempMessage->pMessage, tempMessage );
				}
				else
				{
					message = tempMessage;
				}
			}
			else
			{
				if( pName[0] == '#' )
				{
					pName = Localize( pName + 1 );
				}

				// If we couldnt find it in the titles.txt, just create it
				message = AllocMessage( pName );

				message->effect = 2;
				message->r1 = message->g1 = message->b1 = message->a1 = 100;
				message->r2 = 240;
				message->g2 = 110;
				message->b2 = 0;
				message->a2 = 0;
				message->x = -1;		// Centered
				message->y = 0.7;
				message->fadein = 0.01;
				message->fadeout = 1.5;
				message->fxtime = 0.25;
				message->holdtime = 5;
			}

			// safety check - don't add empty messages
            if ( !message->pMessage || message->pMessage[0] == '\0' ) 
            {
                // clean up custom messages
                if ( !strcmp(message->pName, "Custom") ) 
                {
                    delete[] message->pMessage;
                }
                return; // bail out if message is empty
            }

			for ( j = 0; j < maxHUDMessages; j++ )
			{
				if ( m_pMessages[j] )
				{
					// is this message already in the list
					if ( !strcmp( message->pMessage, m_pMessages[j]->pMessage ) )
					{
						if( !strcmp( message->pName, "Custom" ) )
						{
							delete[] message->pMessage;
						}
						return;
					}

					// get rid of any other messages in same location (only one displays at a time)
					if ( fabs( message->y - m_pMessages[j]->y ) < 0.0001 && fabs( message->x - m_pMessages[j]->x ) < 0.0001 )
					{
						if( !strcmp( m_pMessages[j]->pName, "Custom" ) )
						{
							delete[] m_pMessages[j]->pMessage;
						}
						m_pMessages[j] = NULL;
					}
				}
			}

			m_pMessages[i] = message;
			m_startTime[i] = time;
			return;
		}
	}
}


int CHudMessage::MsgFunc_HudText( const char *pszName,  int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );

	char *pString = reader.ReadString();

	MessageAdd( pString, gHUD.m_flTime );
	// Remember the time -- to fix up level transitions
	m_parms.time = gHUD.m_flTime;

	// Turn on drawing
	m_iFlags |= HUD_DRAW;

	return 1;
}


int CHudMessage::MsgFunc_GameTitle( const char *pszName,  int iSize, void *pbuf )
{
	m_pGameTitle = TextMessageGet( "GAMETITLE" );
	if ( m_pGameTitle != NULL )
	{
		m_gameTitleTime = gHUD.m_flTime;

		// Turn on drawing
		m_iFlags |= HUD_DRAW;
	}

	return 1;
}

void CHudMessage::MessageAdd(client_textmessage_t * newMessage )
{
	client_textmessage_t *message;

	m_parms.time = gHUD.m_flTime;

	// Turn on drawing
	m_iFlags |= HUD_DRAW;

	if( !strcmp( newMessage->pName, "Custom" ) ) // Hey, it's mine way of detecting allocated message
	{
		message = AllocMessage( newMessage->pMessage, newMessage );
	}
	else
	{
		message = newMessage;
	}

	for ( int i = 0; i < maxHUDMessages; i++ )
	{
		if ( !m_pMessages[i] )
		{
			m_pMessages[i] = message;
			m_startTime[i] = gHUD.m_flTime;
			return;
		}
	}

}


int CHudMessage::MsgFunc_HudTextPro( const char *pszName, int iSize, void *pbuf )
{
	const char *sz;
	int hint;
	BufferReader reader( pszName, pbuf, iSize );
	sz = reader.ReadString();
	hint = reader.ReadByte();

	MessageAdd(sz, gHUD.m_flTime/*, hint, Newfont*/); // TODO

	// Remember the time -- to fix up level transitions
	m_parms.time = gHUD.m_flTime;

	// Turn on drawing
	m_iFlags |= HUD_DRAW;
	return 1;
}

int CHudMessage::MsgFunc_HudTextArgs( const char *pszName, int iSize, void *pbuf )
{
	/*BufferReader reader( pszName, pbuf, iSize );

	const char *sz = reader.ReadString();
	int hint = reader.ReadByte();

	MessageAdd(sz, gHUD.m_flTime, hint, Newfont); // TODO

	// Remember the time -- to fix up level transitions
	m_parms.time = gHUD.m_flTime;

	// Turn on drawing
	if ( !(m_iFlags & HUD_ACTIVE) )
		m_iFlags |= HUD_ACTIVE;*/

	return 1;
}

client_textmessage_t *CHudMessage::AllocMessage( const char *text, client_textmessage_t *copyFrom )
{
	const int MAX_CUSTOM_MESSAGES = 16;
	const int MAX_CUSTOM_MESSAGES_MASK = (MAX_CUSTOM_MESSAGES-1);
	static client_textmessage_t	g_pCustomMessage[MAX_CUSTOM_MESSAGES] = { };
	static int g_iCustomMessageMod = 0;

	client_textmessage_t *ret = &g_pCustomMessage[g_iCustomMessageMod];
	g_iCustomMessageMod = (g_iCustomMessageMod + 1) & MAX_CUSTOM_MESSAGES_MASK;

	if( copyFrom )
	{
		*ret = *copyFrom;
	}

	if( text )
	{
		int len = strlen( text );
		char *szCustomText = new char[len];
		strcpy( szCustomText, text );

		ret->pName = "Custom";
		ret->pMessage = szCustomText;
	}

	return ret;
}
