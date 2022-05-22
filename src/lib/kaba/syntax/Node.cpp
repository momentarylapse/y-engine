/*
 * Node.cpp
 *
 *  Created on: 18.02.2019
 *      Author: michi
 */
#include "../kaba.h"
#include "../asm/asm.h"
#include "../../file/file.h"
#include <stdio.h>

namespace kaba {

string kind2str(NodeKind kind) {
	if (kind == NodeKind::PLACEHOLDER)
		return "placeholder";
	if (kind == NodeKind::VAR_LOCAL)
		return "local";
	if (kind == NodeKind::VAR_GLOBAL)
		return "global";
	if (kind == NodeKind::FUNCTION)
		return "function name";
	if (kind == NodeKind::CONSTANT)
		return "constant";
	if (kind == NodeKind::CONSTANT_BY_ADDRESS)
		return "constant by addr";
	if (kind == NodeKind::CALL_FUNCTION)
		return "call";
	if (kind == NodeKind::CALL_RAW_POINTER)
		return "raw pointer call";
	if (kind == NodeKind::CALL_INLINE)
		return "inline";
	if (kind == NodeKind::CALL_VIRTUAL)
		return "virtual call";
	if (kind == NodeKind::STATEMENT)
		return "statement";
	if (kind == NodeKind::OPERATOR)
		return "operator";
	if (kind == NodeKind::ABSTRACT_TOKEN)
		return "token";
	if (kind == NodeKind::ABSTRACT_OPERATOR)
		return "abstract operator";
	if (kind == NodeKind::ABSTRACT_ELEMENT)
		return "abstract element";
	if (kind == NodeKind::ABSTRACT_CALL)
		return "abstract call";
	if (kind == NodeKind::ABSTRACT_TYPE_SHARED)
		return "shared";
	if (kind == NodeKind::ABSTRACT_TYPE_OWNED)
		return "owned";
	if (kind == NodeKind::ABSTRACT_TYPE_POINTER)
		return "pointer";
	if (kind == NodeKind::ABSTRACT_TYPE_LIST)
		return "list";
	if (kind == NodeKind::ABSTRACT_TYPE_DICT)
		return "dict";
	if (kind == NodeKind::ABSTRACT_TYPE_CALLABLE)
		return "callable type";
	if (kind == NodeKind::ABSTRACT_VAR)
		return "var";
	if (kind == NodeKind::BLOCK)
		return "block";
	if (kind == NodeKind::ADDRESS_SHIFT)
		return "address shift";
	if (kind == NodeKind::ARRAY)
		return "array element";
	if (kind == NodeKind::DYNAMIC_ARRAY)
		return "dynamic array element";
	if (kind == NodeKind::POINTER_AS_ARRAY)
		return "pointer as array element";
	if (kind == NodeKind::REFERENCE)
		return "address operator";
	if (kind == NodeKind::DEREFERENCE)
		return "dereferencing";
	if (kind == NodeKind::DEREF_ADDRESS_SHIFT)
		return "deref address shift";
	if (kind == NodeKind::CLASS)
		return "class";
	if (kind == NodeKind::ARRAY_BUILDER)
		return "array builder";
	if (kind == NodeKind::ARRAY_BUILDER_FOR)
		return "array builder for";
	if (kind == NodeKind::ARRAY_BUILDER_FOR_IF)
		return "array builder for if";
	if (kind == NodeKind::DICT_BUILDER)
		return "dict builder";
	if (kind == NodeKind::TUPLE)
		return "tuple";
	if (kind == NodeKind::TUPLE_EXTRACTION)
		return "tuple extract";
	if (kind == NodeKind::CONSTRUCTOR_AS_FUNCTION)
		return "constructor function";
	if (kind == NodeKind::VAR_TEMP)
		return "temp";
	if (kind == NodeKind::DEREF_VAR_TEMP)
		return "deref temp";
	if (kind == NodeKind::REGISTER)
		return "register";
	if (kind == NodeKind::ADDRESS)
		return "address";
	if (kind == NodeKind::MEMORY)
		return "memory";
	if (kind == NodeKind::LOCAL_ADDRESS)
		return "local address";
	if (kind == NodeKind::LOCAL_MEMORY)
		return "local memory";
	if (kind == NodeKind::DEREF_REGISTER)
		return "deref register";
	if (kind == NodeKind::LABEL)
		return "label";
	if (kind == NodeKind::DEREF_LABEL)
		return "deref label";
	if (kind == NodeKind::GLOBAL_LOOKUP)
		return "global lookup";
	if (kind == NodeKind::DEREF_GLOBAL_LOOKUP)
		return "deref global lookup";
	if (kind == NodeKind::IMMEDIATE)
		return "immediate";
	if (kind == NodeKind::DEREF_LOCAL_MEMORY)
		return "deref local";
	return format("UNKNOWN KIND: %d", (int)kind);
}


string Node::signature(const Class *ns) const {
	//string t = (kind == NodeKind::ABSTRACT_TOKEN) ? " " : type->cname(ns) + " ";
	string t = type->cname(ns) + " ";
	if (kind == NodeKind::PLACEHOLDER)
		return "";
	if (kind == NodeKind::VAR_LOCAL)
		return t + as_local()->name;
	if (kind == NodeKind::VAR_GLOBAL)
		return t + as_global()->name;
	if (kind == NodeKind::FUNCTION)
		return t + as_func()->cname(ns);
	if (kind == NodeKind::CONSTANT)
		return t + as_const()->str();
	if (kind == NodeKind::CALL_FUNCTION)
		return as_func()->signature(ns);
	if (kind == NodeKind::CALL_RAW_POINTER)
		return t + "(...)";
	if (kind == NodeKind::CALL_INLINE)
		return as_func()->signature(ns);
	if (kind == NodeKind::CALL_VIRTUAL)
		return as_func()->signature(ns);
	if (kind == NodeKind::CONSTRUCTOR_AS_FUNCTION)
		return as_func()->signature(ns);
	if (kind == NodeKind::STATEMENT)
		return t + as_statement()->name;
	if (kind == NodeKind::OPERATOR)
		return as_op()->sig(ns);
	if (kind == NodeKind::ABSTRACT_TOKEN)
		return "<" + as_token() + ">";
	if (kind == NodeKind::ABSTRACT_OPERATOR)
		return "<" + as_abstract_op()->name + ">";
	if (kind == NodeKind::BLOCK)
		return "";//p2s(as_block());
	if (kind == NodeKind::ADDRESS_SHIFT)
		return t + i2s(link_no);
	if (kind == NodeKind::ARRAY)
		return t;
	if (kind == NodeKind::DYNAMIC_ARRAY)
		return t;
	if (kind == NodeKind::POINTER_AS_ARRAY)
		return t;
	if (kind == NodeKind::REFERENCE)
		return t;
	if (kind == NodeKind::DEREFERENCE)
		return t;
	if (kind == NodeKind::DEREF_ADDRESS_SHIFT)
		return t + i2s(link_no);
	if (kind == NodeKind::CLASS)
		return as_class()->cname(ns);
	if (kind == NodeKind::REGISTER)
		return t + Asm::get_reg_name((Asm::RegID)link_no);
	if (kind == NodeKind::ADDRESS)
		return t + i2h(link_no, config.pointer_size);
	if (kind == NodeKind::MEMORY)
		return t + i2h(link_no, config.pointer_size);
	if (kind == NodeKind::LOCAL_ADDRESS)
		return t + i2h(link_no, config.pointer_size);
	if (kind == NodeKind::LOCAL_MEMORY)
		return t + i2h(link_no, config.pointer_size);
	return t + i2s(link_no);
}

string Node::str(const Class *ns) const {
	return "<" + kind2str(kind) + ">  " + signature(ns);
}


void Node::show(const Class *ns) const {
	string orig;
	msg_write(str(ns) + orig);
	msg_right();
	for (auto p: params)
		if (p)
			p->show(ns);
		else
			msg_write("<NULL>");
	msg_left();
}





// policy:
//  don't change after creation...
//  edit the tree by shallow copy, relink to old parameters
//  relinked params count as "new" Node!
// ...(although, Block are allowed to be edited)
Node::Node(NodeKind _kind, int64 _link_no, const Class *_type, bool _const, int _token_id) {
	type = _type;
	kind = _kind;
	link_no = _link_no;
	is_const = _const;
	token_id = _token_id;
}

Node::~Node() {
}

Node *Node::modifiable() {
	is_const = false;
	return this;
}

Node *Node::make_const() {
	is_const = true;
	return this;
}

bool Node::is_call() const {
	return (kind == NodeKind::CALL_FUNCTION) or (kind == NodeKind::CALL_VIRTUAL) or (kind == NodeKind::CALL_RAW_POINTER);
}

bool Node::is_function() const {
	return (kind == NodeKind::CALL_FUNCTION) or (kind == NodeKind::CALL_VIRTUAL) or (kind == NodeKind::CALL_INLINE) or (kind == NodeKind::CONSTRUCTOR_AS_FUNCTION);
}

Block *Node::as_block() const {
	return (Block*)this;
}

Function *Node::as_func() const {
	return (Function*)link_no;
}

const Class *Node::as_class() const {
	return (const Class*)link_no;
}

Constant *Node::as_const() const {
	return (Constant*)link_no;
}

Operator *Node::as_op() const {
	return (Operator*)link_no;
}
void *Node::as_func_p() const {
	return (void*)as_func()->address;
}

// will be the address at runtime...(not the current location...)
void *Node::as_const_p() const {
	return as_const()->address;
}

void *Node::as_global_p() const {
	return as_global()->memory;
}

Variable *Node::as_global() const {
	return (Variable*)link_no;
}

Variable *Node::as_local() const {
	return (Variable*)link_no;
}

Statement *Node::as_statement() const {
	return (Statement*)link_no;
}

AbstractOperator *Node::as_abstract_op() const {
	return (AbstractOperator*)link_no;
}

string Node::as_token() const {
	return reinterpret_cast<SyntaxTree*>((int_p)link_no)->expressions.get_token(token_id);
}

void Node::set_instance(shared<Node> p) {
#ifndef NDEBUG
	if (params.num == 0)
		msg_write("no inst...dfljgkldfjg");
#endif
	params[0] = p;
	if (this->_pointer_ref_counter > 1) {
		msg_write("iii");
		msg_write(msg_get_trace());
	}
}

void Node::set_type(const Class *t) {
	type = t;
	if (this->_pointer_ref_counter > 1) {
		msg_write("ttt");
		msg_write(msg_get_trace());
	}
}

void Node::set_num_params(int n) {
	params.resize(n);
	if (this->_pointer_ref_counter > 1) {
		msg_write("nnn");
		msg_write(msg_get_trace());
	}
}

void Node::set_param(int index, shared<Node> p) {
#ifndef NDEBUG
	/*if ((index < 0) or (index >= uparams.num)){
		show();
		throw Exception(format("internal: Node.set_param...  %d %d", index, params.num), "", 0);
	}*/
#endif
	params[index] = p;
#if 0
	if (this->_pointer_ref_counter > 1) {
		msg_write("ppp");
		msg_write(msg_get_trace());
	}
#endif
}

shared<Node> Node::shallow_copy() const {
	auto r = new Node(kind, link_no, type, is_const, token_id);
	r->params = params;
	return r;
}

shared<Node> Node::ref(const Class *override_type) const {
	const Class *t = override_type ? override_type : type->get_pointer();

	shared<Node> c = new Node(NodeKind::REFERENCE, 0, t, false, token_id);
	c->set_num_params(1);
	c->set_param(0, const_cast<Node*>(this));
	return c;
}

shared<Node> Node::deref(const Class *override_type) const {
	if (!override_type)
		override_type = type->param[0];
	shared<Node> c = new Node(NodeKind::DEREFERENCE, 0, override_type, is_const, token_id);
	c->set_num_params(1);
	c->set_param(0, const_cast<Node*>(this));
	return c;
}

shared<Node> Node::shift(int64 shift, const Class *type, int token_id) const {
	shared<Node> c = new Node(NodeKind::ADDRESS_SHIFT, shift, type, is_const, token_id);
	c->set_num_params(1);
	c->set_param(0, const_cast<Node*>(this));
	return c;
}

shared<Node> Node::deref_shift(int64 shift, const Class *type, int token_id) const {
	shared<Node> c = new Node(NodeKind::DEREF_ADDRESS_SHIFT, shift, type, is_const, token_id);
	c->set_num_params(1);
	c->set_param(0, const_cast<Node*>(this));
	return c;
}


}

