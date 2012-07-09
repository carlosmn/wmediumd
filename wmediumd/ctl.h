/*
 *	wmediumd, wireless medium simulator for mac80211_hwsim kernel module
 *	Copyright (c) 2012 Carlos Martín Nieto <cmn@dwim.me>.
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version 2
 *	of the License, or (at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 
 *	02110-1301, USA.
 */

#ifndef _WMEDIUM_H_
#define _WMEDIUM_H_

/**
 * BYE
 *
 * The dispatcher is shutting down. The client should start cleaning
 * up its own state. The test program should show stats about received
 * and sent messages.
 */
#define CTL_BYE "BYE"

#endif // _WMEDIUM_H_
