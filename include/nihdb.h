/*
_________  ____ __________________________________________________
\_   ___ \|    |   \______   \_   _____|______  \______  \______  \
/    \  \/|    |   /|    |  _/|    __)_    /    /   /    /   /    /
\     \___|    |  / |    |   \|        \  /    /   /    /   /    /
\________/|______/  |________/_________/ /____/   /____/   /____/

Developer: Kobus van Schoor alias Cube777
Email: pbscube@gmail.com

You are free to use or modify any part of this code as long as you acknowledge
the original developer.
*/

/*
This is an easy-to-use plaintext database parser and editor. It supports nested section
hierarchies with no limit as to how many sections can be nested in each other. It also supports
indenting with the 'Tab' char, and will auto-indent the files if you desire for it to do so.

The structure of the database is as follows:
#This is a comment
[section]
	$variable=value
	#Nested comment
	[section/subsection]
		$variable=value

Please note that no variables can be declared outside of a section and that a variable's value cannot be an empty string.
The comment, delimiter and variable char can be modified by changing the 'DAT_DELIMITER', 'DAT_VARSTART' and 'DAT_VARSTART'
values respectively defined below.
*/

#pragma once

#include <string>
#include <vector>

namespace nihdb
{
class dataBase
{
public:
	dataBase(std::string filename);
	//Reparse the database file, returns false if there was
	//errors in the database file
	bool Reparse();

	//Write any changes made to the database to the its file,
	//you need to call this function if _alwaysApply is not set
	//to true otherwise any changed made to the file will be lost
	bool ApplyChanges();

	//Set _alwaysApply, if set to true, any changes made to the database will
	//be automatically written to the file. _alwaysApply defaults to false
	void SetAlwaysApply(bool value);

	//If set to true, applying the changes to the file automatically indent
	//the lines according to their hierarchy level. Defaults to true
	void SetAutoIndent(bool value);

	//Create a new section, if it is a subsection, you can specify it by separating
	//the subsection form the parent section with the following syntax:
	//"parentSection/subsection"
	//Any amount of subsections can exist as long as each has a unique name in the scope
	//of it's parent
	bool CreateSection(std::string name);

	//Creates a new variable, returns false if variable name is already used or section
	//does not exist.varName can be any plaintext name, section can be used in the exact
	//same manner as described above at CreateSection(). Returns false if the value string
	//is empty
	bool CreateVar(std::string section, std::string varName, std::string value = "empty");

	//Changes the value of a variable returns false if either the section or variable
	//could not be found. Also returns false if the new value is an empty string
	bool ChangeVarValue(std::string section, std::string varName, std::string newValue);

	//Appends a comment to the file
	void AddComment(std::string comment);

	//Returns a variables value as a string, returns an empty string if the variable
	//could not be found
	std::string ReturnVar(std::string section, std::string varName);

	//Deletes a section and all data encompassed in it, up to the next section with the same
	//hierarchy level. Returns false if the section specified does not exist
	bool DeleteSection(std::string section);

	//Deletes a variable form the database, returns false if the section or variable specified
	//does not exist
	bool DeleteVar(std::string section, std::string varName);

	//Returns if the database is parsed or not
	bool IsParsed();

private:
	class FileData
	{
	public:
		enum LineType {Section, Variable, Comment, BlankLine};
		bool CreateSection(std::string section);
		bool CreateVar(std::string section, std::string varName, std::string value);
		bool CreateVar(std::string section, std::string rawLine);
		bool ChangeVarValue(std::string section, std::string varName, std::string newValue);
		void AddCommentOrBlank(std::string comment);
		std::string ReturnVar(std::string section, std::string varName);
		bool DeleteSection(std::string section);
		bool DeleteVar(std::string section, std::string varName);
		void ClearAll();
		std::string ReturnRawLine(int line, bool addIndent = false);
		int AmountOfLines();

	private:
		struct RawLine {
		public:
			RawLine(std::string rawLine);

			std::string _rawline;
			int _level;
			std::vector<std::string> _sections;
			std::vector<std::string> _parentSection;
			LineType _lineType;
			std::string _varName;
			std::string _value;

			bool _parsed;
			void Update();
		};

		//Returns NULL if key does not exist
		RawLine* ReturnLineRef(std::string section, std::string varName);
		bool DoesSectionExist(std::string section);
		bool DoesVarExist(std::string section, std::string varName);
		std::vector<std::string> ParseSectionStr(std::string section);
		std::string CombineSectVect(std::vector<std::string> section);

		std::vector<RawLine> _lines;
	};

	FileData _fileData;
	std::string _filename;
	bool _parsed;
	bool _alwaysApply;
	bool _autoIndent;
};
}
