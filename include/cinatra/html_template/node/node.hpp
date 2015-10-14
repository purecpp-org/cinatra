/*
 * Node.h
 *
 *  Created on: 2014
 *      Author: jc
 */
#pragma once

#include <cinatra/json/json.hpp>
#include <cinatra/html_template/context/context.hpp>
#include <cinatra/html_template/io/writer.hpp>

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

namespace cinatra
{
	class Context;
	class Fragment;
	class Writer;

	class Node
	{
	public:
		virtual void render(Writer * stream, Context * /*context*/) const
		{
			stream->write("*** NOT IMPLEMENTED ***");
		}

		void renderChildren(Writer * stream, Context * context,
			std::vector< std::shared_ptr< Node > > children = {}) const
		{
			if (!children.size())
				children = m_children;
			std::for_each(children.begin(), children.end(),
				std::bind(&Node::render, std::placeholders::_1, stream, context));
		}

		virtual void processFragment(Fragment const * /*fragment*/){}
		void addChild(Node * child)
		{
			m_children.push_back(std::shared_ptr< Node >(child));
		}

		bool createsScope() const{ return m_createsScope; }

		virtual void enterScope(){}
		virtual void exitScope(std::string const & /*endTag*/){}

		virtual std::string name() const{ return "Node"; }

		std::vector< std::shared_ptr< Node > > const & children(){ return m_children; }

		template< class T >
		std::vector< std::shared_ptr< T > > childrenByName(std::string const & name)
		{
			std::vector< std::shared_ptr< Node > > nodes;
			std::copy_if(m_children.begin(), m_children.end(), std::back_inserter(nodes),
				std::bind(std::equal_to< std::string >(), std::bind(&Node::name, std::placeholders::_1), name));
			std::vector< std::shared_ptr< T > > result;
			typedef std::shared_ptr< T >(*DynamicPointerCast)(std::shared_ptr< Node > const &);
			DynamicPointerCast caster = std::dynamic_pointer_cast<T>;
			std::transform(nodes.begin(), nodes.end(), std::back_inserter(result), caster);
			return result;
		}

		virtual ~Node(){}

	protected:
		Node(bool createsScope = false)
			: m_createsScope(createsScope)
		{}
		virtual Json resolveInContext(std::string const & name, Context const * context) const
		{
			return context->resolve(name);
		}

	protected:
		std::vector< std::shared_ptr< Node > > m_children;
		bool m_createsScope;
	};

}

