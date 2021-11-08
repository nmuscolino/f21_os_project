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
			   //System.err.println(reportCounter);
			   reportList.nextLine();
			   String[] reports = new String[reportCounter];
			   String tmp;			   
   
			   int i = 0;
			   while(reportList.hasNextLine()) {
				tmp = reportList.nextLine();
				//System.err.println(tmp);
				//System.err.println("i = " + i);
				if (tmp.isEmpty()) break;
				else reports[i] = tmp;
				//System.err.println(reports[i]);
				i++;
			   } 

 		     //load specs and create threads for each report
				 DebugLog.log("Load specs and create threads for each report\nStart thread to request, process and print reports");

			   Vector<ReportGenerator> reportVector = new Vector<ReportGenerator>(reportCounter);
			   for (int j = 0; j < reportCounter; j++) {
				//System.err.println("Constructing ReportGenerator " + j);
				reportVector.add(new ReportGenerator(reports[j], j + 1, reportCounter));
				//System.err.println("Starting ReportGenerator " + j);
				reportVector.elementAt(j).start(); 
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
