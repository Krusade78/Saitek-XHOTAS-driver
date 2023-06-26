Public Class InstalarDriversX36U

    Public Sub Procesar()
        Me.ParentForm.Hide()
        If IO.File.Exists("drvapi.exe") Then
            Dim newProc As Diagnostics.Process = Diagnostics.Process.Start("drvapi.exe", "3 0 0")
            newProc.WaitForExit()
            Select Case newProc.ExitCode
                Case 1
                    Me.PictureBox5.Image = My.Resources.OK
                Case 2
                    Me.PictureBox5.Image = My.Resources.OK
                    Me.PictureBox3.Image = My.Resources.OK
                    Me.ParentForm.DialogResult = DialogResult.OK
                    Threading.Thread.Sleep(2000)
                    Me.ParentForm.Close()
                    Exit Sub
            End Select
        End If
        Me.ParentForm.Show()
        Button1.Visible = True
    End Sub

    Private Sub Button1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button1.Click
        Me.ParentForm.Close()
    End Sub

    Private Sub InstalarDriversX36U_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        Select Case (System.Globalization.CultureInfo.CurrentCulture.LCID And &HFF)
            Case &HC
                'nombre = "french.xml"
            Case &H10
                'nombre = "italian.xml"
            Case &HA
                Label5.Text = "Controlador XHOTAS Virtual de teclado y ratón"
                Label3.Text = "Controlador USB del XHOTAS"
                Button1.Text = "Finalizar"
            Case &H7
                'nombre = "german.xml"
        End Select
    End Sub
End Class
