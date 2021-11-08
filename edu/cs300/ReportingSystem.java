package edu.cs300;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.Enumeration;
import java.util.Scanner;
import java.util.Vector;

public class ReportingSystem {


	public ReportingSystem() {
	  DebugLog.log("Starting Reporting System");
	}

	public int loadReportJobs() {
		int reportCounter = 0;
		try {
			   //Prepare to read list of reports from specified file
			   File file = new File ("report_list.txt");
			   Scanner reportList = new Scanner(file);

			   //Read total number of reports from input file as an integer
			   reportCounter = reportList.nextInt();
			   reportList.nextLine();

			   //Total number of reports is known, so initialize an array 
			   //of the appropriate size to hold the names of all specification files
			   String[] reports = new String[reportCounter];
			   String tmp;			   
   
			   //Read the report list to obtain the names of all specification files
			   int i = 0;
			   while(reportList.hasNextLine()) {
				tmp = reportList.nextLine();
				if (tmp.isEmpty()) break;
				else reports[i] = tmp;
				i++;
			   } 

 		     //load specs and create threads for each report
				 DebugLog.log("Load specs and create threads for each report\nStart thread to request, process and print reports");

			   //Create a vector to hold all ReportGenerator threads
			   Vector<ReportGenerator> reportVector = new Vector<ReportGenerator>(reportCounter);
			   for (int j = 0; j < reportCounter; j++) {
				//Create a new ReportGenerator object. The reports index will be j + 1
				reportVector.add(new ReportGenerator(reports[j], j + 1, reportCounter));
				//Start the newly created ReportGenerator thread
				reportVector.elementAt(j).start(); 
			   }

			   //Reading is completed. Close the scanner
			   reportList.close();
		} catch (FileNotFoundException ex) {
			  System.out.println("FileNotFoundException triggered:"+ex.getMessage());
		}
		return reportCounter;

	}

	public static void main(String[] args) throws FileNotFoundException {


		   ReportingSystem reportSystem= new ReportingSystem();
		   int reportCount = reportSystem.loadReportJobs();


	}

}
