/* 
 * File:   Console.cpp
 * Author: jcrada
 *
 * Created on 30 November 2013, 10:55 AM
 */

#include "fl/Console.h"

#include "fl/Headers.h"

#include <algorithm>
#include <vector>
#include <utility>
#include <stdlib.h>
#include <fstream>

namespace fl {
    const std::string Console::KW_INPUT_FILE = "-i";
    const std::string Console::KW_INPUT_FORMAT = "-if";
    const std::string Console::KW_OUTPUT_FILE = "-o";
    const std::string Console::KW_OUTPUT_FORMAT = "-of";
    const std::string Console::KW_EXAMPLE = "-example";
    const std::string Console::KW_DECIMALS = "-decimals";
    const std::string Console::KW_DATA_INPUT = "-d";
    const std::string Console::KW_DATA_MAXIMUM = "-dmaximum";
    const std::string Console::KW_DATA_SHOW_HEADERS = "-dheaders";
    const std::string Console::KW_DATA_SHOW_INPUTS = "-dinputs";

    std::vector<Console::Option> Console::availableOptions() {
        std::vector<Console::Option> options;
        options.push_back(Option(KW_INPUT_FILE, "inputfile", "file to import your engine from"));
        options.push_back(Option(KW_INPUT_FORMAT, "format", "format of the file to import (fll | fis | fcl)"));
        options.push_back(Option(KW_OUTPUT_FILE, "outputfile", "file to export your engine to"));
        options.push_back(Option(KW_OUTPUT_FORMAT, "format", "format of the file to export (fll | fld | cpp | java | fis | fcl)"));
        options.push_back(Option(KW_EXAMPLE, "letter", "if not inputfile, built-in example to use as engine: (m)amdani or (t)akagi-sugeno"));
        options.push_back(Option(KW_DECIMALS, "number", "number of decimals to write floating-poing values"));
        options.push_back(Option(KW_DATA_INPUT, "datafile", "if exporting to fld, file of input values to evaluate your engine on"));
        options.push_back(Option(KW_DATA_MAXIMUM, "number", "if exporting to fld without datafile, maximum number of results to export"));
        options.push_back(Option(KW_DATA_SHOW_HEADERS, "boolean", "if true and exporting to fld, include headers"));
        options.push_back(Option(KW_DATA_SHOW_INPUTS, "boolean", "if true and exporting to fld, include input values"));
        return options;
    }

    std::string Console::usage() {
        std::vector<Console::Option> options = availableOptions();
        std::ostringstream ss;
        ss << "========================================\n";
        ss << "fuzzylite: a fuzzy logic control library\n";
        ss << "version: " << fuzzylite::longVersion() << "\n";
        ss << "author: " << fuzzylite::author() << "\n";
        ss << "========================================\n";
        ss << "usage: fuzzylite inputfile outputfile\n";
        ss << "   or: fuzzylite ";
        for (std::size_t i = 0; i < options.size(); ++i) {
            ss << "[" << options.at(i).key << " " << options.at(i).value << "] ";
        }
        ss << "\n\nwhere:\n";
        for (std::size_t i = 0; i < options.size(); ++i) {
            std::string spacedKey(12, ' ');
            std::string key = options.at(i).key;
            std::copy(key.begin(), key.end(), spacedKey.begin());

            std::string spacedValue(13, ' ');
            std::string value = options.at(i).value;
            std::copy(value.begin(), value.end(), spacedValue.begin());

            std::string description = options.at(i).description;

            ss << spacedKey << spacedValue << description << "\n";
        }


        ss << "\n";
        ss << "Visit http://www.fuzzylite.com for more information.";
        return ss.str();
    }

    std::map<std::string, std::string> Console::parse(int argc, char** argv) {
        if ((argc - 1) % 2 != 0) {
            throw fl::Exception("[option error] incomplete number of parameters [key value]", FL_AT);
        }
        std::map<std::string, std::string> options;
        for (int i = 1; i < argc - 1; i += 2) {
            std::string key = std::string(argv[i]);
            std::string value = std::string(argv[i + 1]);
            options[key] = value;
        }
        if (options.size() == 1) {
            std::map<std::string, std::string>::const_iterator it = options.begin();
            if (it->first.at(0) != '-') {
                options[KW_INPUT_FILE] = it->first;
                options[KW_OUTPUT_FILE] = it->second;
            }
        } else {
            std::vector<Console::Option> validOptions = availableOptions();

            for (std::map<std::string, std::string>::const_iterator it = options.begin();
                    it != options.end(); ++it) {
                bool isValid = false;
                for (std::size_t i = 0; i < validOptions.size(); ++i) {
                    std::string key = validOptions.at(i).key;
                    if (key == it->first) {
                        isValid = true;
                        break;
                    }
                }
                if (not isValid) {
                    throw fl::Exception("[option error] option <" + it->first + "> not recognized", FL_AT);
                }
            }
        }
        return options;
    }

    void Console::process(const std::map<std::string, std::string>& options) {
        std::map<std::string, std::string>::const_iterator it;

        it = options.find(KW_DECIMALS);
        if (it != options.end()) {
            fl::fuzzylite::setDecimals((int) fl::Op::toScalar(it->second));
        }

        std::string example;
        std::string inputFormat;
        std::ostringstream textEngine;

        it = options.find(KW_EXAMPLE);

        bool isExample = (it != options.end());

        if (isExample) {
            example = it->second;
            Engine* engine;
            if (example == "m" or example == "mamdani") {
                engine = mamdani();
            } else if (example == "t" or example == "ts" or example == "takagi-sugeno") {
                engine = takagiSugeno();
            } else {
                throw fl::Exception("[option error] example <" + example + "> not available", FL_AT);
            }
            inputFormat = "fll";
            textEngine << FllExporter().toString(engine);
            delete engine;

        } else {
            it = options.find(KW_INPUT_FILE);
            if (it == options.end()) {
                throw fl::Exception("[option error] no input file specified", FL_AT);
            }
            std::string inputFilename = it->second;
            std::ifstream inputFile(inputFilename.c_str());
            if (not inputFile.is_open()) {
                throw fl::Exception("[file error] file <" + inputFilename + "> could not be opened", FL_AT);
            }
            std::string line;
            while (std::getline(inputFile, line)) {
                textEngine << line << std::endl;
            }
            inputFile.close();

            it = options.find(KW_INPUT_FORMAT);
            if (it != options.end()) {
                inputFormat = it->second;
            } else {
                std::size_t extensionIndex = inputFilename.find_last_of(".");
                if (extensionIndex != std::string::npos) {
                    inputFormat = inputFilename.substr(extensionIndex + 1);
                } else {
                    throw fl::Exception("[format error] unspecified format of input file", FL_AT);
                }
            }
        }

        std::string outputFilename;
        it = options.find(KW_OUTPUT_FILE);
        if (it != options.end()) {
            outputFilename = it->second;
        }

        std::string outputFormat;
        it = options.find(KW_OUTPUT_FORMAT);
        if (it != options.end()) {
            outputFormat = it->second;
        } else {
            std::size_t extensionIndex = outputFilename.find_last_of(".");
            if (extensionIndex != std::string::npos) {
                outputFormat = outputFilename.substr(extensionIndex + 1);
            } else {
                throw fl::Exception("[format error] unspecified format of output file", FL_AT);
            }
        }


        if (outputFilename.empty()) {
            process(textEngine.str(), std::cout, inputFormat, outputFormat, options);
        } else {
            std::ofstream writer(outputFilename.c_str());
            if (not writer.is_open()) {
                throw fl::Exception("[file error] file <" + outputFilename + "> could not be created", FL_AT);
            }
            process(textEngine.str(), writer, inputFormat, outputFormat, options);
            writer.flush();
            writer.close();
        }
    }

    template <typename T>
    void Console::process(const std::string& input, T& writer,
            const std::string& inputFormat, const std::string& outputFormat,
            const std::map<std::string, std::string>& options) {
        Importer* importer = NULL;
        Exporter* exporter = NULL;
        Engine* engine = NULL;

        try {
            if ("fll" == inputFormat) {
                importer = new FllImporter;
            } else if ("fcl" == inputFormat) {
                importer = new FclImporter;
            } else if ("fis" == inputFormat) {
                importer = new FisImporter;
            } else {
                throw fl::Exception("[import error] format <" + inputFormat + "> "
                        "not supported", FL_AT);
            }

            engine = importer->fromString(input);

            if ("fld" == outputFormat) {
                std::map<std::string, std::string>::const_iterator it;

                FldExporter fldExporter;
                bool showHeaders = true;
                if ((it = options.find(KW_DATA_SHOW_HEADERS)) != options.end()) {
                    showHeaders = ("true" == it->second);
                }
                bool showInputValues = true;
                if ((it = options.find(KW_DATA_SHOW_INPUTS)) != options.end()) {
                    showInputValues = ("true" == it->second);
                }
                if ((it = options.find(KW_DATA_INPUT)) != options.end()) {
                    std::ifstream dataFile(it->second.c_str());
                    if (not dataFile.is_open()) {
                        throw fl::Exception("[export error] file <" + it->second + "> could not be opened", FL_AT);
                    }
                    if (showHeaders) {
                        writer << "#" << fldExporter.header(engine) << "\n";
                    }
                    try {
                        std::string line;
                        int lineNumber = 0;
                        while (std::getline(dataFile, line)) {
                            ++lineNumber;
                            std::vector<scalar> inputValues;
                            try {
                                fldExporter.parse(Op::trim(line), inputValues);
                            } catch (fl::Exception& ex) {
                                ex.append(" at line <" + Op::str(lineNumber) + ">");
                                throw;
                            }
                            if (inputValues.empty()) continue;
                            if ((int) inputValues.size() != engine->numberOfInputVariables()) {
                                std::ostringstream ex;
                                ex << "[export error] engine has <" << engine->numberOfInputVariables() << "> "
                                        "input variables, but input data provides <" << inputValues.size() << "> values "
                                        "at line <" << lineNumber << ">";
                                throw fl::Exception(ex.str(), FL_AT);
                            }
                            fldExporter.toWriter(engine, writer, inputValues, fldExporter.getSeparator(), showInputValues);
                            writer << "\n";
                            writer.flush();
                        }
                    } catch (std::exception& ex) {
						(void)ex;
                        dataFile.close();
                        throw;
                    }

                } else {
                    int maximum = 1024;
                    if ((it = options.find(KW_DATA_MAXIMUM)) != options.end()) {
                        maximum = (int) fl::Op::toScalar(it->second);
                    }
                    if (showHeaders) {
                        if (showInputValues) {
                            writer << "#" << fldExporter.header(engine) << "\n";
                        } else {
                            writer << "#" << fldExporter.header(engine->outputVariables()) << "\n";
                        }

                    }
                    fldExporter.toWriter(engine, writer, maximum, fldExporter.getSeparator(), showInputValues);
                }
            } else {
                if ("fll" == outputFormat) {
                    exporter = new FllExporter;
                } else if ("fcl" == outputFormat) {
                    exporter = new FclExporter;
                } else if ("fis" == outputFormat) {
                    exporter = new FisExporter;
                } else if ("cpp" == outputFormat) {
                    exporter = new CppExporter;
                } else if ("java" == outputFormat) {
                    exporter = new JavaExporter;
                } else {
                    throw fl::Exception("[export error] format <" + outputFormat + "> "
                            "not supported", FL_AT);
                }
                writer << exporter->toString(engine);
            }
        } catch (std::exception& ex) {
			(void)ex;
            if (importer) delete importer;
            if (exporter) delete exporter;
            if (engine) delete engine;
            throw;
        }
        if (importer) delete importer;
        if (exporter) delete exporter;
        if (engine) delete engine;
    }

    template void Console::process(const std::string& input, std::ostringstream& writer,
            const std::string& inputFormat, const std::string& outputFormat,
            const std::map<std::string, std::string>& options);

    template void Console::process(const std::string& input, std::ofstream& writer,
            const std::string& inputFormat, const std::string& outputFormat,
            const std::map<std::string, std::string>& options);

    Engine* Console::mamdani() {
        Engine* engine = new Engine("simple-dimmer");

        InputVariable* ambient = new InputVariable("Ambient", 0, 1);
        ambient->addTerm(new Triangle("DARK", .0, .25, .5));
        ambient->addTerm(new Triangle("MEDIUM", .25, .5, .75));
        ambient->addTerm(new Triangle("BRIGHT", .5, .75, 1));
        engine->addInputVariable(ambient);


        OutputVariable* power = new OutputVariable("Power", 0, 2);
        power->setDefaultValue(fl::nan);
        power->addTerm(new Triangle("LOW", 0.0, 0.5, 1));
        power->addTerm(new Triangle("MEDIUM", 0.5, 1, 1.5));
        power->addTerm(new Triangle("HIGH", 1, 1.5, 2));
        engine->addOutputVariable(power);

        RuleBlock* ruleblock = new RuleBlock();
        ruleblock->addRule(Rule::parse("if Ambient is DARK then Power is HIGH", engine));
        ruleblock->addRule(Rule::parse("if Ambient is MEDIUM then Power is MEDIUM", engine));
        ruleblock->addRule(Rule::parse("if Ambient is BRIGHT then Power is LOW", engine));

        engine->addRuleBlock(ruleblock);

        engine->configure("", "", "Minimum", "Maximum", "Centroid");

        return engine;
    }

    Engine* Console::takagiSugeno() {
        Engine* engine = new Engine("approximation of sin(x)/x");

        fl::InputVariable* inputX = new fl::InputVariable("inputX");
        inputX->setRange(0, 10);
        inputX->addTerm(new fl::Triangle("NEAR_1", 0, 1, 2));
        inputX->addTerm(new fl::Triangle("NEAR_2", 1, 2, 3));
        inputX->addTerm(new fl::Triangle("NEAR_3", 2, 3, 4));
        inputX->addTerm(new fl::Triangle("NEAR_4", 3, 4, 5));
        inputX->addTerm(new fl::Triangle("NEAR_5", 4, 5, 6));
        inputX->addTerm(new fl::Triangle("NEAR_6", 5, 6, 7));
        inputX->addTerm(new fl::Triangle("NEAR_7", 6, 7, 8));
        inputX->addTerm(new fl::Triangle("NEAR_8", 7, 8, 9));
        inputX->addTerm(new fl::Triangle("NEAR_9", 8, 9, 10));
        engine->addInputVariable(inputX);


        fl::OutputVariable* outputFx = new fl::OutputVariable("outputFx");
        outputFx->setRange(-1, 1);
        outputFx->setDefaultValue(fl::nan);
        outputFx->setLockValidOutput(true); //To use its value with diffFx
        outputFx->addTerm(new Constant("f1", 0.84));
        outputFx->addTerm(new Constant("f2", 0.45));
        outputFx->addTerm(new Constant("f3", 0.04));
        outputFx->addTerm(new Constant("f4", -0.18));
        outputFx->addTerm(new Constant("f5", -0.19));
        outputFx->addTerm(new Constant("f6", -0.04));
        outputFx->addTerm(new Constant("f7", 0.09));
        outputFx->addTerm(new Constant("f8", 0.12));
        outputFx->addTerm(new Constant("f9", 0.04));
        engine->addOutputVariable(outputFx);

        fl::OutputVariable* trueFx = new fl::OutputVariable("trueFx");
        trueFx->setRange(fl::nan, fl::nan);
        trueFx->setLockValidOutput(true); //To use its value with diffFx
        trueFx->addTerm(fl::Function::create("fx", "sin(inputX)/inputX", engine));
        engine->addOutputVariable(trueFx);

        fl::OutputVariable* diffFx = new fl::OutputVariable("diffFx");
        diffFx->addTerm(fl::Function::create("diff", "fabs(outputFx-trueFx)", engine));
        diffFx->setRange(fl::nan, fl::nan);
        //        diffFx->setLockValidOutput(true); //To use in input diffPreviousFx
        engine->addOutputVariable(diffFx);

        fl::RuleBlock* block = new fl::RuleBlock();
        block->addRule(fl::Rule::parse("if inputX is NEAR_1 then outputFx is f1", engine));
        block->addRule(fl::Rule::parse("if inputX is NEAR_2 then outputFx is f2", engine));
        block->addRule(fl::Rule::parse("if inputX is NEAR_3 then outputFx is f3", engine));
        block->addRule(fl::Rule::parse("if inputX is NEAR_4 then outputFx is f4", engine));
        block->addRule(fl::Rule::parse("if inputX is NEAR_5 then outputFx is f5", engine));
        block->addRule(fl::Rule::parse("if inputX is NEAR_6 then outputFx is f6", engine));
        block->addRule(fl::Rule::parse("if inputX is NEAR_7 then outputFx is f7", engine));
        block->addRule(fl::Rule::parse("if inputX is NEAR_8 then outputFx is f8", engine));
        block->addRule(fl::Rule::parse("if inputX is NEAR_9 then outputFx is f9", engine));
        block->addRule(fl::Rule::parse("if inputX is any then trueFx is fx and diffFx is diff", engine));
        engine->addRuleBlock(block);

        engine->configure("", "", "AlgebraicProduct", "AlgebraicSum", "WeightedAverage");

        return engine;
    }

    void Console::exportAllExamples(const std::string& from, const std::string& to) {
        std::vector<std::string> examples;
        examples.push_back("/mamdani/AllTerms");
        examples.push_back("/mamdani/SimpleDimmer");
        examples.push_back("/mamdani/matlab/mam21");
        examples.push_back("/mamdani/matlab/mam22");
        examples.push_back("/mamdani/matlab/shower");
        examples.push_back("/mamdani/matlab/tank");
        examples.push_back("/mamdani/matlab/tank2");
        examples.push_back("/mamdani/matlab/tipper");
        examples.push_back("/mamdani/matlab/tipper1");
        examples.push_back("/mamdani/octave/investment_portfolio");
        examples.push_back("/mamdani/octave/mamdani_tip_calculator");
        examples.push_back("/takagi-sugeno/approximation");
        examples.push_back("/takagi-sugeno/SimpleDimmer");
        examples.push_back("/takagi-sugeno/matlab/fpeaks");
        examples.push_back("/takagi-sugeno/matlab/invkine1");
        examples.push_back("/takagi-sugeno/matlab/invkine2");
        examples.push_back("/takagi-sugeno/matlab/juggler");
        examples.push_back("/takagi-sugeno/matlab/membrn1");
        examples.push_back("/takagi-sugeno/matlab/membrn2");
        examples.push_back("/takagi-sugeno/matlab/slbb");
        examples.push_back("/takagi-sugeno/matlab/slcp");
        examples.push_back("/takagi-sugeno/matlab/slcp1");
        examples.push_back("/takagi-sugeno/matlab/slcpp1");
        examples.push_back("/takagi-sugeno/matlab/sltbu_fl");
        examples.push_back("/takagi-sugeno/matlab/sugeno1");
        examples.push_back("/takagi-sugeno/matlab/tanksg");
        examples.push_back("/takagi-sugeno/matlab/tippersg");
        examples.push_back("/takagi-sugeno/octave/cubic_approximator");
        examples.push_back("/takagi-sugeno/octave/heart_disease_risk");
        examples.push_back("/takagi-sugeno/octave/linear_tip_calculator");
        examples.push_back("/takagi-sugeno/octave/sugeno_tip_calculator");
        examples.push_back("/tsukamoto/tsukamoto");

        std::string sourceBase = "/home/jcrada/Development/fl/fuzzylite/examples/original";
        std::string targetBase = "/tmp/fl/";

        Importer* importer;
        if (from == "fll") importer = new FllImporter;
        else if (from == "fis") importer = new FisImporter;
        else if (from == "fcl") importer = new FclImporter;
        else throw fl::Exception("[examples error] unrecognized format <" + from + "> to import", FL_AT);

        Exporter* exporter;
        if (to == "fll") exporter = new FllExporter;
        else if (to == "fld") exporter = new FldExporter(" ");
        else if (to == "fcl") exporter = new FclExporter;
        else if (to == "fis") exporter = new FisExporter;
        else if (to == "cpp") exporter = new CppExporter;
        else if (to == "java") exporter = new JavaExporter;
        else throw fl::Exception("[examples error] unrecognized format <" + to + "> to export", FL_AT);

        std::vector<std::pair<Exporter*, Importer*> > tests;
        tests.push_back(std::pair<Exporter*, Importer*>(new FllExporter, new FllImporter));
        tests.push_back(std::pair<Exporter*, Importer*>(new FclExporter, new FclImporter));
        tests.push_back(std::pair<Exporter*, Importer*>(new FisExporter, new FisImporter));
        for (std::size_t i = 0; i < examples.size(); ++i) {
            FL_LOG("Processing " << (i + 1) << "/" << examples.size() << ": " << examples.at(i));
            std::ostringstream ss;
            std::string input = sourceBase + examples.at(i) + "." + from;
            std::ifstream source(input.c_str());
            if (source.is_open()) {
                std::string line;
                while (source.good()) {
                    std::getline(source, line);
                    ss << line << "\n";
                }
                source.close();
            } else throw fl::Exception("[examples error] file not found: " + input, FL_AT);

            Engine* engine = importer->fromString(ss.str());

            for (std::size_t t = 0; t < tests.size(); ++t) {
                std::string out = tests.at(t).first->toString(engine);
                Engine* copy = tests.at(t).second->fromString(out);
                std::string out_copy = tests.at(t).first->toString(copy);

                if (out != out_copy) {
                    std::ostringstream ss;
                    ss << "[imex error] different results <"
                            << importer->name() << "," << exporter->name() << "> "
                            "at " + examples.at(t) + "." + from + ":\n";
                    throw fl::Exception(ss.str(), FL_AT);
                }
                delete copy;
            }

            std::string output = targetBase + examples.at(i) + "." + to;
            std::ofstream target(output.c_str());
            if (target.is_open()) {
                if (to == "cpp") {
                    target << "#include <fl/Headers.h>\n\n"
                            << "int main(int argc, char** argv){\n"
                            << exporter->toString(engine)
                            << "\n}\n";
                } else if (to == "java") {
                    std::string className = examples.at(i).substr(examples.at(i).find_last_of('/') + 1);
                    target << "import com.fuzzylite.*;\n"
                            << "import com.fuzzylite.defuzzifier.*;\n"
                            << "import com.fuzzylite.factory.*;\n"
                            << "import com.fuzzylite.hedge.*;\n"
                            << "import com.fuzzylite.imex.*;\n"
                            << "import com.fuzzylite.norm.*;\n"
                            << "import com.fuzzylite.norm.s.*;\n"
                            << "import com.fuzzylite.norm.t.*;\n"
                            << "import com.fuzzylite.rule.*;\n"
                            << "import com.fuzzylite.term.*;\n"
                            << "import com.fuzzylite.variable.*;\n\n"
                            << "public class " << Op::makeValidId(className) << "{\n"
                            << "public static void main(String[] args){\n"
                            << exporter->toString(engine)
                            << "\n}\n}\n";
                } else {
                    target << exporter->toString(engine);
                }
                target.close();
            }
            delete engine;
        }
        delete importer;
        delete exporter;
        for (std::size_t i = 0; i < tests.size(); ++i) {
            delete tests.at(i).first;
            delete tests.at(i).second;
        }
    }

    int Console::main(int argc, char** argv) {
        (void) argc;
        (void) argv;
        if (argc <= 1) {
            std::cout << usage() << std::endl;
            return EXIT_SUCCESS;
        }
        if (argc == 2 and "export-examples" == std::string(argv[1])) {
            fuzzylite::setDecimals(3);
            exportAllExamples("fis", "fll");
            exportAllExamples("fis", "fcl");
            exportAllExamples("fis", "fis");
            exportAllExamples("fis", "cpp");
            exportAllExamples("fis", "java");
            fuzzylite::setDecimals(8);
            exportAllExamples("fis", "fld");
            return 0;
        }

        try {
            std::map<std::string, std::string> options = parse(argc, argv);
            process(options);
        } catch (std::exception& ex) {
            std::cout << ex.what() << "\nBACKTRACE:\n" <<
                    fl::Exception::btCallStack() << std::endl;
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }


}