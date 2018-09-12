// Certificate of Authenticity
//
// EGP-405-01 Networking for Online Games
// Lab 2
// 7-30-2018
//
// Vedant Chaudhari, 1532077
// Lucas Spiker
//
// We certify that this work is entirely our own.The assessor of this project may reproduce this project 
// and provide copies to other academic staff, and/or communicate a copy of this project to a plagiarism 
// - checking service, which may retain a copy of the project on its database.

#pragma once

#include <stdio.h>
#include <stdlib.h>

// Inspired by https://github.com/skorks/c-linked-list

// Struct definitions
typedef struct node {
	char username[31];

	struct node* next;
} Node;

// Node Functions
Node* createNode(char username[31]);

typedef struct list {
	Node* head;
} List;

// List Functions
void append(char username[31], List* list);
void remove(char username[31], List* list);
void print(List* list);
void destroy(List* list);