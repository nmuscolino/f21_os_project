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

			   File file = new File ("report_list.txt");

			   Scanner reportList = new Scanner(file);
			   reportCounter = reportList.nextInt();
			   reportList.nextLine();
			   String[] reports = new String[reportCounter];
			   
			   int i = 0;
			   while(reportList.hasNextLine()) {
				reports[i] = reportList.nextLine();
				i++;
			   } 

 		     //load specs and create threads for each report
				 DebugLog.log("Load specs and create threads for each report\nStart thread to request, process and print reports");

			   Vector<ReportGenerator> reportVector = new Vector<ReportGenerator>(reportCounter);
			   for (int i = 0; i < reportCounter; i++) {
				reportVector.add(new ReportGenerator(reports[i], i + 1, reportCounter);
				reportVector.elementAt(i).start(); 
			   }

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
