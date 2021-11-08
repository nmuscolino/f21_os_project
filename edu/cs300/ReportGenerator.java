package edu.cs300;

import java.io.File;
import java.io.FileWriter;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Enumeration;
import java.util.Scanner;
import java.util.Vector;

public class ReportGenerator extends Thread {
	String inputFileName;
	int index;
	int reportCount;
	String reportTitle;
	String searchString;
	String outputFileName;
	Vector<ReportField> reportFields;

	public ReportGenerator(String inputFileName, int id, int reportCount) {
		//System.err.println("In the ReportGenerator constructor");
		this.inputFileName = inputFileName;
		this.index = id;
		this.reportCount = reportCount;
		
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
			//System.err.println("Getting fields for Thread " + this.index);
			while(scanner.hasNext()) {
				tempFieldString = scanner.nextLine();
				//System.err.println(tempFieldString);
				//Retrieve index of first column corresponding to the current field
				//System.err.println(tempFieldString.substring(0, (tempFieldString.indexOf('-'))));
				//System.err.println(tempFieldString.indexOf('-'));
				tempBeginningCol = Integer.parseInt(tempFieldString.substring(0, (tempFieldString.indexOf('-'))));
				//Retrieve index of the last column corresponding to the current field
				tempEndCol = Integer.parseInt(tempFieldString.substring((tempFieldString.indexOf('-') + 1), (tempFieldString.indexOf(','))));
				//Retrieve the name of the heading corresponding to the current field
				tempColHeading = tempFieldString.substring((tempFieldString.indexOf(',') + 1));
				//System.err.println((tempBeginningCol - 1) + "\n" + (tempEndCol - 1) +"\n" + tempColHeading);
				reportFields.add(new ReportField(tempBeginningCol - 1, tempEndCol - 1, tempColHeading));
			}
		scanner.close();
		} catch (FileNotFoundException e) {
			System.err.println("Error reading from input file.");
		}
	}

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
		for (int i = 0; i < this.reportFields.size(); i++) {
			parsedMessage = parsedMessage + message.substring(reportFields.elementAt(i).startIndex, (reportFields.elementAt(i).endIndex + 1)) + "\t";
		}
		parsedMessage = parsedMessage + "\n";
		return parsedMessage;
	}

	public void run() {
		//System.err.println("In run method of ReportGenerator " + this.index);
		MessageJNI.writeReportRequest(this.index, this.reportCount, this.searchString);

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
			String receivedMessage;
			do {
				receivedMessage = MessageJNI.readReportRecord(this.index);
				if (receivedMessage.length() == 0) {
					writer.write("\n");
				} else {
					receivedMessage = parseMessage(receivedMessage);
					writer.write(receivedMessage);
				}
			} while (receivedMessage.length() > 0);

			writer.flush();
			writer.close();
		} catch (IOException e) {
			System.err.println("Error writing to file");
		}
	}
	
}
