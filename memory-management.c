// Meant for atoms that are not really ready yet (not used right now)
#define T_UNINITIALIZED 0
// Normal atom types
#define T_NUM 1
#define T_SYM 2
#define T_STR 3
#define T_PAIR 4
// Usef for singleton atoms. Albeit these are singletons the type should be used for comparisons.
#define T_NIL 5
#define T_TRUE 5
#define T_FALSE 6

struct atom_s {
	uint8_t type;
	int64_t num;
	char *sym;
	char *str;
	struct {
		struct atom_s *first, *rest;
	} pair;
};
typedef struct atom_s atom_t;


// Singleton atoms

atom_t *allocator_nil_atom, *allocator_true_atom, *allocator_false_atom;

atom_t* alloc_atom(uint8_t type);

void allocator_init(){
	allocator_nil_atom = alloc_atom(T_NIL);
	allocator_true_atom = alloc_atom(T_TRUE);
	allocator_false_atom = alloc_atom(T_FALSE);
}

atom_t* get_nil_atom(){
	return allocator_nil_atom;
}

atom_t* get_true_atom(){
	return allocator_true_atom;
}

atom_t* get_false_atom(){
	return allocator_false_atom;
}


// Normal atoms

atom_t* alloc_num(){
	atom_t *ptr = malloc(sizeof(atom_t));
	ptr->type = T_NUM;
	return ptr;
}

atom_t* alloc_sym(){
	atom_t *ptr = malloc(sizeof(atom_t));
	ptr->type = T_SYM;
	return ptr;
}

atom_t* alloc_str(){
	atom_t *ptr = malloc(sizeof(atom_t));
	ptr->type = T_STR;
	return ptr;
}

atom_t* alloc_pair(){
	atom_t *ptr = malloc(sizeof(atom_t));
	ptr->type = T_PAIR;
	return ptr;
}

atom_t* alloc_atom(uint8_t type){
	atom_t *ptr = malloc(sizeof(atom_t));
	ptr->type = type;
	return ptr;
}