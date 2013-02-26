/*
 *	wmediumd, wireless medium simulator for mac80211_hwsim kernel module
 *	Copyright (c) 2011 cozybit Inc.
 *
 *	Author:	Javier Lopez	<jlopex@cozybit.com>
 *		Javier Cardona	<javier@cozybit.com>
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
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 *	MA  02110-1301, USA.
 */


#ifndef PROBABILITY_H_
#define PROBABILITY_H_

#include "mac_address.h"

struct node;

struct edge {
	float loss;
	struct node *node;
};

struct node {
	struct mac_address addr;
	struct edge neighbours[]; /* dynamically allocated */
};


#define MATRIX_PROB(MATRIX,LIMITS,X,Y,Z) (MATRIX)[((X)+(Y)*(LIMITS))+((LIMITS)*(LIMITS)*(Z))]


void put_mac_address(struct mac_address addr, int pos);
struct mac_address *get_mac_address(int pos);
void print_mac_address_array();

void fill_prob_matrix(double *aMatrix,double aValue);
void print_prob_matrix (double *aMatrix);

double generate_random_double();
int find_pos_by_mac_address(struct mac_address *addr);
double find_prob_by_addrs_and_rate (double *aMatrix,struct mac_address *src, 
				    struct mac_address *dst, int rate_idx);

double *init_probability(int num);



#endif /* PROBABILITY_H_ */
