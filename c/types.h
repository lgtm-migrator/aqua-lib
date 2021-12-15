#if !defined(__AQUA_LIB__TYPES)
#define __AQUA_LIB__TYPES

// TODO exception handling system (perhaps a global exception queue?)

typedef struct type_t type_t; // forward declaration

typedef struct {
	type_t* type;
} object_t; // abstract "class"

struct type_t {
	const char* name;

	// unary operators

	uint64_t (*len) (void* _x);
	void (*del) (void* _x);

	// binary operators
	
	unsigned (*eq) (void* _x, void* _y);
	void* (*add) (void* _x, void* _y);
};

#define TYPE_OP_ERROR fprintf(stderr, "[TYPES] ERROR '%s' does not have a '%s' operator\n", x->type->name, __func__);

// unary operator

static uint64_t len(void* _x) {
	object_t* x = _x;

	if (!x) {
		return -1;
	}

	if (!x->type->len) {
		TYPE_OP_ERROR
		return -1;
	}

	return x->type->len(x);
}

static void del(void* _x) {
	object_t* x = _x;

	if (!x) {
		return;
	}

	if (x->type->del) {
		x->type->del(x);
	}

	free(x);
}

// binary operators

static unsigned eq(void* _x, void* _y) {
	object_t* x = _x;
	object_t* y = _y;

	if (!x && !y) {
		// two objects are equal if they're both NULL
		return 1;
	}

	if (!x || !y) {
		return 0;
	}

	if (!x->type->eq) {
		TYPE_OP_ERROR
		return 0;
	}

	return x->type->eq(x, y);
}

static void* add(void* _x, void* _y) {
	object_t* x = _x;
	object_t* y = _y;

	if (!x || !y) {
		return 0;
	}

	if (!x->type->add) {
		TYPE_OP_ERROR
		return 0;
	}

	return x->type->add(x, y);
}

#endif