#include "input_reader.h"

#include <gtest/gtest.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

struct test_domain {
	std::string domain_name;
	bool is_valid;
};

void ReadTestDomainsFromFile(std::vector<test_domain>& test_domains, const std::string& file_name) {
	// Open file
	std::ifstream infile(file_name);

	// Read file line by line
	std::string line;
	while (std::getline(infile, line)) {
		test_domains.push_back({line, true});

		// Check for every character in the line if it is valid
		for (const char& test_char : test_domains.back().domain_name) {
			test_domains.back().is_valid &= (test_char >= 'A' && test_char <= 'Z') ||
							(test_char >= 'a' && test_char <= 'z') ||
			                                (test_char >= '0' && test_char <= '9') ||
			                                test_char == '-' || test_char == '.' || test_char == '_';
		}

		// Check if the domain name exceeds the maximum size
		if (test_domains.back().domain_name.length() > DOMAIN_NAME_MAX_SIZE - 1) {
			test_domains.back().is_valid = false;

			// The input reader limits the length of the domain name to
			// DOMAIN_NAME_MAX_SIZE including null terminator
			test_domains.back().domain_name.resize(DOMAIN_NAME_MAX_SIZE - 1);
		}

		if (test_domains.back().domain_name.length() == 0) {
			test_domains.back().is_valid = false;
		}
	}

	// Close file
	infile.close();
}

// Test file read and validate the contents
TEST(InputReaderTest, FileTest) {
	// First read file lines with basic and slow function
	std::string file_name = "test_list.txt";

	std::vector<test_domain> test_domains;;
	ReadTestDomainsFromFile(test_domains, file_name);

	// Open file for use in the input reader
	std::unique_ptr<FILE, int (*)(FILE*)> fptr(fopen(file_name.c_str(), "r"),
	    [](FILE* fp) -> int {
		    if (fp)
			    return ::fclose(fp);
		    return EOF;
	    });
		
	if (fptr.get() == NULL)
		FAIL() << "Cannot open file";

	InputReader reader(fptr.get());

	DomainInputInfo domain_info;
	char buf[DOMAIN_NAME_MAX_SIZE];
	domain_info.buf = buf;

	// Loop over every domain found with ReadTestDomainsFromFile
	// and check if input reader yields the same result
	for (const test_domain& domain : test_domains) {
		// Input reader works asynchronously, wait for result to be available
		ReadDomainResult res = ReadDomainResult::NotAvailable;
		while (res == ReadDomainResult::NotAvailable) {
			res = reader.GetDomain(domain_info);
		}

		// File end is only expected after all domains from the test
		// array are finshed
		if (res == ReadDomainResult::FileEnd)
			FAIL() << "File end not expected already";

		EXPECT_STREQ(domain_info.buf, domain.domain_name.c_str());
		EXPECT_EQ(domain.is_valid, res == ReadDomainResult::Success ? true : false);
	}

	// Check if the last result is FileEnd
	ReadDomainResult res = ReadDomainResult::NotAvailable;
	while (res == ReadDomainResult::NotAvailable) {
		res = reader.GetDomain(domain_info);
	}
	EXPECT_EQ(res, ReadDomainResult::FileEnd);
}
