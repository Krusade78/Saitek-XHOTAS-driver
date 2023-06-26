Public Class Form1
    Private Shared uninst As Boolean = True

    Public Shared Function Main(ByVal cmd() As String) As Integer
        If My.Application.CommandLineArgs.Count > 0 Then
            If My.Application.CommandLineArgs(0).ToLower = "/i" Then
                uninst = False
                If Form1.ShowDialog() = Windows.Forms.DialogResult.OK Then
                    Return 0
                Else
                    Return 1
                End If
            End If
        End If

        If Form1.ShowDialog() = Windows.Forms.DialogResult.OK Then
            Return 0
        Else
            Return 1
        End If
    End Function

    Private Sub Form1_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        If uninst Then ' desinstalar
            Me.Hide()
            If IO.File.Exists("drvapi.exe") Then
                Dim newProc As Diagnostics.Process = Diagnostics.Process.Start("drvapi.exe")
                newProc.WaitForExit()
                If newProc.ExitCode = 1 Then
                    Me.DialogResult = Windows.Forms.DialogResult.OK
                    Me.Close()
                    Exit Sub
                End If
            End If

            Dim i0 As New Desinstalar()
            i0.Dock = DockStyle.Fill
            Me.Controls.Add(i0)
        Else
            Dim i1 As New Instalar1
            i1.Dock = DockStyle.Fill
            Me.Controls.Add(i1)
        End If
    End Sub

#Region "X36G"
    Private selTeclado As Integer = -1
    Private selX35 As Boolean
    Private gameports As Boolean = True
    Public Sub ProcesarX36G(ByVal x35 As Boolean)
        Me.Controls.Clear()
        selX35 = x35
        Dim i2 As New InstalarTeclado
        i2.Dock = DockStyle.Fill
        Me.Controls.Add(i2)
    End Sub
    Public Sub ProcesarTeclados(ByVal ret As Integer)
        selTeclado = ret
        Me.Controls.Clear()
        If gameports Then
            Dim i3 As New InstalarGameport
            i3.Dock = DockStyle.Fill
            Me.Controls.Add(i3)
        Else
            Dim i3 As New InstalarDriversX36M
            i3.Dock = DockStyle.Fill
            Me.Controls.Add(i3)
            i3.Procesar(selTeclado)
        End If
    End Sub
    Public Sub ProcesarGameports(ByVal selGameport As Integer)
        Me.Controls.Clear()
        Dim i4 As New InstalarDriversX36G
        i4.Dock = DockStyle.Fill
        Me.Controls.Add(i4)
        i4.Procesar(selX35, selTeclado, selGameport)
    End Sub
#End Region

    Public Sub ProcesarX36M()
        gameports = False
        ProcesarX36G(False)
    End Sub

    Public Sub ProcesarX45()
        Me.Controls.Clear()
        Dim i5 As New InstalarDriversX36U
        i5.Dock = DockStyle.Fill
        Me.Controls.Add(i5)
        i5.Procesar()
    End Sub
End Class
