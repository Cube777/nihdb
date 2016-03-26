#include "../include/nihdb.h"

#define DAT_COMMENT '#'
#define DAT_VARSTART '$'
#define DAT_DELIMITER '='
#include <fstream>

using namespace nihdb;

dataBase::dataBase(std::string filename) :
	_filename(filename),
	_alwaysApply(false),
	_autoIndent(true)
{
	_parsed = this->Reparse();
}

bool dataBase::Reparse()
{
	std::ifstream fileRead(_filename.c_str());
	//Return false if there was an error reading the file
	if (!fileRead.is_open())
		return false;

	_fileData.ClearAll();
	std::string tempRead;
	std::string lastSection;
	std::string component;
	while (std::getline(fileRead, tempRead)) {
		int startLet = 0;
		component.clear();
		for (; tempRead[startLet] == '\t'; startLet++);

		if (tempRead == "")
			tempRead = "\n";
		char _type = tempRead[startLet];

		if (_type == '[') {
			for (int i = startLet; i < tempRead.size(); i++)
				component += tempRead[i];

			//Return false if there was an error creating the section
			if (!_fileData.CreateSection(component))
				return false;

			lastSection.clear();
			for (int i = startLet + 1; i + 1 < tempRead.size(); i++)
				lastSection += tempRead[i];
			continue;
		}

		if (_type == DAT_VARSTART) {
			component = tempRead;
			for (int i = 0; i < startLet; i++)
				component.erase(component.begin());
			//Return false if there was an error creating the variable
			if (!_fileData.CreateVar(lastSection, component))
				return false;

			continue;
		}

		if ((_type == DAT_COMMENT) ||
		        (_type == '\n')) {
			component = tempRead;
			for (int i = 0; i < startLet; i++)
				component.erase(component.begin());
			_fileData.AddCommentOrBlank(component);
			continue;
		}
		return false;
	}
	return true;
}

bool dataBase::ApplyChanges()
{
	std::ofstream fileWrite(_filename.c_str());

	//Return false if file could not be opened for writing
	if (!fileWrite)
		return false;

	std::string lineRead;
	for (int line = 0; line < _fileData.AmountOfLines(); line++) {
		lineRead= _fileData.ReturnRawLine(line, _autoIndent);
		fileWrite << lineRead << '\n';
	}

	return true;
}

void dataBase::SetAlwaysApply(bool value)
{
	_alwaysApply = value;
	return;
}

void dataBase::SetAutoIndent(bool value)
{
	_autoIndent = value;
	return;
}

bool dataBase::CreateSection(std::string name)
{
	bool success = _fileData.CreateSection(name);
	if (success && _alwaysApply)
		return ApplyChanges();
	return success;
}

bool dataBase::CreateVar(std::string section, std::string varName, std::string value /* = "empty" */)
{
	if (value.empty())
		return false;

	bool success = _fileData.CreateVar(section, varName, value);
	if (success && _alwaysApply)
		return ApplyChanges();
	return success;
}

bool dataBase::ChangeVarValue(std::string section, std::string varName, std::string newValue)
{
	if (newValue.empty())
		return false;

	bool success = _fileData.ChangeVarValue(section, varName, newValue);
	if (success && _alwaysApply)
		return ApplyChanges();
	return success;
}

void dataBase::AddComment(std::string comment)
{
	if (comment.empty())
		_fileData.AddCommentOrBlank("\n");
	else
		_fileData.AddCommentOrBlank(DAT_COMMENT + comment);
	if (_alwaysApply)
		ApplyChanges();
}

std::string dataBase::ReturnVar(std::string section, std::string varName)
{
	return _fileData.ReturnVar(section, varName);
}

bool dataBase::DeleteSection(std::string section)
{
	bool succes = _fileData.DeleteSection(section);
	if (succes && _alwaysApply)
		return ApplyChanges();
	return succes;
}

bool dataBase::DeleteVar(std::string section, std::string varName)
{
	bool succes = _fileData.DeleteVar(section, varName);
	if (succes && _alwaysApply)
		return ApplyChanges();
	return succes;
}

bool dataBase::IsParsed()
{
	return _parsed;
}

bool dataBase::FileData::CreateSection(std::string section)
{
	RawLine *newLine;
	if (section[0] == '[')
		newLine = new RawLine(section);
	else
		newLine = new RawLine("[" + section + "]");

	if (!newLine->_parsed)
		return false;

	if (DoesSectionExist(CombineSectVect(newLine->_sections)))
		return false;
	auto itr = _lines.begin();

	for (; itr != _lines.end(); itr++) {
		if (itr->_lineType == dataBase::FileData::Section)
			if (itr->_sections == newLine->_parentSection)
				break;
	}
	if (itr != _lines.end())
		itr++;
	for (; itr != _lines.end(); itr++) {
		if (itr->_lineType == dataBase::FileData::Section)
			if (itr->_parentSection != newLine->_parentSection)
				break;
	}

	_lines.insert(itr, *newLine);
	delete newLine;
	return true;
}

bool dataBase::FileData::CreateVar(std::string section, std::string rawLine)
{
	RawLine newLine(rawLine);
	if (!newLine._parsed)
		return false;

	if (DoesVarExist(section, newLine._varName))
		return false;

	newLine._sections = ParseSectionStr(section);
	newLine._level = newLine._sections.size();

	auto itr = _lines.begin();
	for (; itr->_sections != newLine._sections; itr++);
	for (; (itr != _lines.end()) && (itr->_sections == newLine._sections); itr++);

	_lines.insert(itr, newLine);
	return true;
}

bool dataBase::FileData::CreateVar(std::string section, std::string varName, std::string value)
{
	return this->CreateVar(section, DAT_VARSTART + varName + DAT_DELIMITER + value);
}

bool dataBase::FileData::ChangeVarValue(std::string section, std::string varName, std::string newValue)
{
	if (!DoesVarExist(section, varName))
		return false;

	RawLine* line = ReturnLineRef(section, varName);
	line->_value = newValue;
	line->Update();
	return true;
}

void dataBase::FileData::AddCommentOrBlank(std::string comment)
{
	RawLine newLine(comment);

	auto itr = _lines.end();
	if (itr != _lines.begin())
		itr--;

	for (; (itr != _lines.begin()) && (itr->_lineType != dataBase::FileData::Section); itr--);
	if (itr != _lines.begin())
		newLine._sections = itr->_sections;
	newLine._level = newLine._sections.size();
	_lines.push_back(newLine);
	return;
}

std::string dataBase::FileData::ReturnVar(std::string section, std::string varName)
{
	RawLine* returnVal = ReturnLineRef(section, varName);
	if (returnVal != NULL)
		return returnVal->_value;
	else
		return "";
}

bool dataBase::FileData::DeleteSection(std::string section)
{
	//Return false if section does not exist
	if (!DoesSectionExist(section))
		return false;

	auto itr = _lines.begin();

	while (true) {
		for (; itr->_lineType != dataBase::FileData::Section; itr++);
		if (CombineSectVect(itr->_sections) == section)
			break;
		else
			itr++;
	}
	int matchLevel = itr->_level;
	itr = _lines.erase(itr);
	while (true) {
		while ((itr != _lines.end()) && (itr->_lineType != dataBase::FileData::Section))
			itr = _lines.erase(itr);
		if (itr == _lines.end())
			break;

		if (itr->_level <= matchLevel)
			break;

		itr = _lines.erase(itr);
	}
	return true;
}

bool dataBase::FileData::DeleteVar(std::string section, std::string varName)
{
	//Return false if variable doesn't exist
	if (!DoesVarExist(section, varName))
		return false;

	RawLine* target = ReturnLineRef(section, varName);
	auto itr = _lines.begin();

	for (; &*itr != target; itr++);
	_lines.erase(itr);
	return true;
}

void dataBase::FileData::ClearAll()
{
	_lines.clear();
}

std::string dataBase::FileData::ReturnRawLine(int line, bool addIndent /* = false */)
{
	std::string rawLine;
	if (addIndent)
		for (int i = 0; i < _lines[line]._level; i++)
			rawLine += '\t';

	rawLine += _lines[line]._rawline;
	return rawLine;
}

int dataBase::FileData::AmountOfLines()
{
	return _lines.size();
}

dataBase::FileData::RawLine::RawLine(std::string rawLine):
	_parsed(false),
	_level(0),
	_rawline(rawLine)
{
	char _type = rawLine[0];

	if (_type == '[') {
		int currentLet = 1;
		while (true) {
			std::string tempName;
			for (; (currentLet + 1 < rawLine.size()) && (rawLine[currentLet] != '/'); currentLet++)
				tempName += rawLine[currentLet];

			//Return if section name was empty
			if (tempName.size() == 0)
				return;

			if (rawLine[currentLet] == '/') {
				this->_level++;
				this->_sections.push_back(tempName);
				currentLet++;
				continue;
			}

			if (currentLet + 1 == rawLine.size()) {
				//Return if there was no closing bracket
				if (rawLine[currentLet] != ']')
					return;
				else {
					this->_sections.push_back(tempName);
					break;
				}
			}
		}
		this->_lineType = dataBase::FileData::Section;
		this->_parentSection = this->_sections;
		this->_parentSection.pop_back();
		_parsed = true;
		return;
	}

	if (_type == DAT_VARSTART) {
		int currentLet = 1;
		for (; (currentLet < _rawline.size()) && (rawLine[currentLet] != DAT_DELIMITER); currentLet++)
			this->_varName += rawLine[currentLet];

		//Return if no delimiter was not found
		if (currentLet == rawLine.size())
			return;

		currentLet++;
		for (; currentLet < rawLine.size(); currentLet++)
			this->_value += rawLine[currentLet];

		//Return if value is empty
		if (this->_value.empty())
			return;

		this->_lineType = dataBase::FileData::Variable;
		_parsed = true;
		return;
	}

	if (_type == DAT_COMMENT) {
		_lineType = dataBase::FileData::Comment;
		_parsed = true;
		return;
	}
	if (_type == '\n') {
		_lineType = dataBase::FileData::BlankLine;
		_parsed = true;
		return;
	}
	return;
}

void dataBase::FileData::RawLine::Update()
{
	if (_lineType == dataBase::FileData::Variable)
		this->_rawline = DAT_VARSTART + this->_varName + DAT_DELIMITER + this->_value;
	return;
}

dataBase::FileData::RawLine* dataBase::FileData::ReturnLineRef(std::string section, std::string varName)
{
	//Return NULL if key doesn't exist
	if (!DoesVarExist(section, varName))
		return NULL;

	int lineNum = 0;
	while (true) {
		for (; _lines[lineNum]._lineType != dataBase::FileData::Section; lineNum++);

		if (CombineSectVect(_lines[lineNum]._sections) == section)
			break;
		else
			lineNum++;
	}

	for (; _lines[lineNum]._varName != varName; lineNum++);
	return &_lines[lineNum];
}

bool dataBase::FileData::DoesSectionExist(std::string section)
{
	//Return false if section name is empty
	if (section.size() == 0)
		return false;

	std::vector<std::string> _tester = ParseSectionStr(section);

	int lineNum = 0;
	while (true) {
		for (; (lineNum < _lines.size()) && (_lines[lineNum]._lineType != dataBase::FileData::Section); lineNum++);

		if (lineNum == _lines.size())
			return false;

		if (_tester == _lines[lineNum]._sections)
			return true;
		else
			lineNum++;
	}
	return true;
}

bool dataBase::FileData::DoesVarExist(std::string section, std::string varName)
{
	//Return false if section does not exist
	if (!DoesSectionExist(section))
		return false;

	int lineNum = 0;
	while (true) {
		for (; _lines[lineNum]._lineType != dataBase::FileData::Section; lineNum++);

		if (CombineSectVect(_lines[lineNum]._sections) == section)
			break;
		else
			lineNum++;
	}

	std::vector<std::string> _combine = this->ParseSectionStr(section);
	for (; (lineNum < _lines.size()) &&
	        (_lines[lineNum]._sections == _combine) &&
	        (_lines[lineNum]._varName != varName); lineNum++);

	if ((lineNum == _lines.size()) ||
	        (_lines[lineNum]._sections != _combine))
		return false;
	else
		return true;
}

std::vector<std::string> dataBase::FileData::ParseSectionStr(std::string section)
{
	std::vector<std::string> combine;

	int currentLet = 0;
	std::string component;
	if (section[0] == '[') {
		section.erase(section.begin());
		section.pop_back();
	}
	while (true) {
		component.clear();
		for (; (currentLet < section.size()) && (section[currentLet] != '/'); currentLet++)
			component += section[currentLet];
		combine.push_back(component);
		if (currentLet == section.size())
			break;
		else
			currentLet++;
	}
	return combine;
}

std::string dataBase::FileData::CombineSectVect(std::vector<std::string> section)
{
	std::string combine;

	for (int i = 0; i < section.size(); i++) {
		combine += section[i];
		if (i + 1 < section.size())
			combine += '/';
	}

	return combine;
}
