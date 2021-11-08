package edu.cs300;

public class ReportField {
	int startIndex;
	int endIndex;
	String title;

	public ReportField(int startIndex, int endIndex, String title) {
		//System.err.println("Report Field constructor");
		this.startIndex = startIndex;
		this.endIndex = endIndex;
		this.title = title;
	}
}
