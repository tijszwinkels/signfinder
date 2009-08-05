/*
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is HierarchicalStore.h .
 *
 * The Initial Developer of the Original Code is Tijs Zwinkels.
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Tijs Zwinkels <opensource AT tumblecow DOT net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 */

#ifndef HSTORE
#define HSTORE

#include <iostream>
#include <map>
#include <vector>
#include <opencv/cv.h>

using namespace std;

/* Declarations */
class HStoreElement; 
typedef multimap<string,HStoreElement*> ElementMap;
vector<HStoreElement*> it2vec(ElementMap::iterator it, ElementMap::iterator beyond); 


class HStoreElement
{
protected:
	string _type, _name;	
	ElementMap _elements;	
	void* _element;

public:
	HStoreElement(char* type)
	{
		_type = type;
		_element = NULL;
	}

	HStoreElement(char* type, void* element)
	{
		_type = type;
		_element = element;
	}

	HStoreElement(char* type, void* element, char* name)
	{
		_type = type;
		_element = element;
		_name = name;
	}
	virtual ~HStoreElement()
	{
		cout << "HStoreElement destructor called. type: " << _type <<" name: " << _name << endl;
	}

	virtual string display() {return string("");}

	/* Display the tree of elements */
	/*ostream& operator << (ostream& os)
	{
		ElementMap::iterator it = _elements.begin();
		ElementMap::iterator end = _elements.end();
		os << _type << " " << display() << endl;
		if (it != end)
			os << "\\" << endl;
		while (it != end)
		{
			HStoreElement hse("test",NULL);// = it->second;
			os << "|" << hse;
			it++;	
		}

		return os;
	}*/

	void setElement(void* element)
	{
		_element = element;	
	}

	void addElement(char* type, void* element, char* name = NULL)
	{
		HStoreElement* hs = new HStoreElement(type,element,name);	
		_elements.insert(pair<string,HStoreElement*>(type,hs));	
	}

	void* getElement()
	{
		return _element;
	}

	vector<HStoreElement*> getElements(string address)
	{
		vector<HStoreElement*> result ;
		int dotloc = address.find_first_of('.');

		// Local data requested.
		if (dotloc == string::npos)
		{
			ElementMap::iterator find = _elements.find(address);
			ElementMap::iterator ubound = _elements.upper_bound(address);
			result = it2vec(find,ubound);
		}
		// Data in the hierarchy requested.
		else
		{
			string qry = address.substr(0,dotloc);
			vector<HStoreElement*> parentResult = it2vec(_elements.find(qry),_elements.upper_bound(qry));
			cout << "Querying for " << qry << " and requesting " << address.substr(dotloc) << " on the results." << endl;
			for (int i=0; i = parentResult.size(); ++i)
			{
				// Add data returned by the child to the result vector.
				vector<HStoreElement*> childResult = parentResult[i]->getElements(address.substr(dotloc));
				result.insert(result.end(),childResult.begin(),childResult.end());
			}
		}			

		return result;
	}
};

class ImageElement : public HStoreElement
{
public:
	ImageElement(char* type, char* file) : HStoreElement(type)
	{
		init(file);
	}

	virtual ~ImageElement()
	{
		cout << "ImageElement destructor called. type: " << _type <<" name: " << _name << endl;
		cvReleaseImage((IplImage**) &_element);
	}

protected:
	void init(char* file)
	{
		bool failed = false;
		IplImage* img = cvLoadImage(file);
		if (!img)
		{
			cerr << "Could not load file " << file << "creating empty set" << endl;
			failed = true;
		}

		_element = img;
		_name = file;
	}
};

template <class T>
class SpecificElement : public HStoreElement
{
	SpecificElement(char* type, T* element) : HStoreElement(type, (void*) element)
	{}

	SpecificElement(char* type, T* element, char* name)  : HStoreElement(type, (void*) element, name)
	{}

	virtual ~SpecificElement()
	{
		cout << "SpecificElement destructor called. type: " << _type <<" name: " << _name << endl;
		delete((T*) _element);
	}
	T* getElement()
	{
		return (T*) _element;
	}
};

//const char* HStoreElement::MAP = (const char*) 0xee;

/* Support Functions */
vector<HStoreElement*> it2vec(ElementMap::iterator it, ElementMap::iterator end) 
{
	vector<HStoreElement*> result;
	result.reserve(distance(it, end));	
	while (it != end)
		result.push_back(it++->second);
}
		
#endif //HSTORE
