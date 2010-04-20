/*
 * Copyright 2007 Johannes Lehtinen
 * This file is free software; Johannes Lehtinen gives unlimited
 * permission to copy, distribute and modify it.
 */

#ifndef CORE_H_
#define CORE_H_

/**
 * A function that classifies a file. If the classification succeeds then
 * the function should print file description to standard output and
 * return a non-zero value. Otherwise the function must return zero.
 *
 * @param data classified specific runtime data
 * @param path the file path 
 * @return whether classification was successful
 */
typedef int (*classify_func_t)(void *data, const char *path);

/** A short hand typedef for classifier_t structure */
typedef struct classifier_t classifier_t;

/**
 * A container for classifier information.
 */
struct classifier_t {
	
	/** Classifier specific runtime data */
	void *data;
	
	/** The classifying function */
	classify_func_t classify;
}; 

#endif /*CORE_H_*/
