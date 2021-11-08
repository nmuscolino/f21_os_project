package edu.cs300;

import java.io.File;
import java.io.FileWriter;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Enumeration;
import java.util.Scanner;
import java.util.Vector;

/*ReportGenerator is used to create threads that will request records from
 *process_records.c. If matching records are found, it will receive them from
 *process_records.c and parse them for the desired fields. It then outputs the
 *records in a specified report file.
 */ 
public class ReportGenerator extends Thread {
	String inputFileName;
	int index;
	int reportCount;
	String reportTitle;
	String searchString;
	String outputFileName;
	Vector<ReportField> reportFields;

	//Constructor
	public ReportGenerator(String inputFileName, int id, int reportCount) {

		this.inputFileName = inputFileName;
		this.index = id;
		this.reportCount = reportCount;

		//Read the report specifications from the provided input file		
		File inputFile = new File(inputFileName);
		Scanner scanner;
		try {
			scanner = new Scanner(inputFile);
			
			//Read the first three lines of inputFile to retrieve the report title,
			//search string, and the name of the output file of the report
			this.reportTitle = scanner.nextLine();
			this.searchString = scanner.nextLine();
			this.outputFileName = scanner.nextLine();
			
			//Read each subsequent line and parse it to retrieve components of each report field
			this.reportFields = new Vector<ReportField>(0);
			String tempFieldString;
			int tempBeginningCol;
			int tempEndCol;
			String tempColHeading;
			
			//Read the remaining lines of the input file to obtain the desired field ranges and headers
			while(scanner.hasNext()) {
				tempFieldString = scanner.nextLine();
				//Retrieve index of first column corresponding to the current field
				tempBeginningCol = Integer.parseInt(tempFieldString.substring(0, (tempFieldString.indexOf('-'))));
				//Retrieve index of the last column corresponding to the current field
				tempEndCol = Integer.parseInt(tempFieldString.substring((tempFieldString.indexOf('-') + 1), (tempFieldString.indexOf(','))));
				//Retrieve the name of the heading corresponding to the current field
				tempColHeading = tempFieldString.substring((tempFieldString.indexOf(',') + 1));
				
				//Add the field to the reportFields vector for output to the report file
				reportFields.add(new ReportField(tempBeginningCol - 1, tempEndCol - 1, tempColHeading));
			}

			scanner.close();
		} catch (FileNotFoundException e) {
			System.err.println("Error reading from input file.");
		}
	}

	//Creates the file to which the report will be written
	public void createOutputFile(String fileName) {
		try {
			File outputFile = new File(fileName);
			if (outputFile.createNewFile()) {
				System.err.println("File created: " + outputFile.getName());
			} else {
				System.err.println("File already exists.");
			}
		} catch (IOException e) {
			System.err.println("Error creating output file.");
		}
	}

	//Parses received message for necessary fields and builds the string to be written to the output report
	public String parseMessage(String message) {
		String parsedMessage = "";

		//For each field in the output report, concatenate the desired text to be output as a single string
		//This will include any trailing spaces in a selected field
		for (int i = 0; i < this.reportFields.size(); i++) {
			if(message.length() < (reportFields.elementAt(i).endIndex + 1)) {
				//System.err.println("message too short");
				message = String.format("%-" + String.valueOf(reportFields.elementAt(i).endIndex + 1) + "s", message);
				message = message.replace("\n", " ").replace("\r", " ");
				//System.err.println("message now has length " + message.length()); 
			}
			parsedMessage = parsedMessage + message.substring(reportFields.elementAt(i).startIndex, (reportFields.elementAt(i).endIndex + 1)) + "\t";
			parsedMessage = parsedMessage.replace("\n", " ").replace("\r", " ");
		}

		parsedMessage = parsedMessage + "\n";
		return parsedMessage;
	}

	//The code to be executed by each running thread
	public void run() {
		
		//Send a report request to process_records.c
		MessageJNI.writeReportRequest(this.index, this.reportCount, this.searchString);

		//Create the output file for the report
		createOutputFile(this.outputFileName);
		try {
			FileWriter writer = new FileWriter(this.outputFileName);
		
			//Write report title and field headers to output file
			writer.write(this.reportTitle + "\n");
			for(int i = 0; i < this.reportFields.size(); i++) {
				writer.write(this.reportFields.elementAt(i).title + "\t");
			}
			writer.write("\n");		

			//Read received reports and write necessary fields to the output file
			//Receive at least one message from process_records.c to indicate end of report
			String receivedMessage;
			do {
				receivedMessage = MessageJNI.readReportRecord(this.index);
			
				//Check for zero-length record to indicate end of report	
				if (receivedMessage.length() == 0) {
					break;
				} else {
					receivedMessage = parseMessage(receivedMessage);
					writer.write(receivedMessage);
				}
			} while (receivedMessage.length() > 0);

			//Report is complete. Flush and close writer
			writer.flush();
			writer.close();
		} catch (IOException e) {
			System.err.println("Error writing to file");
		}
	}
	
}
